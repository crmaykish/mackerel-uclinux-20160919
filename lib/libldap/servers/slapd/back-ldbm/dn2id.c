/* dn2id.c - routines to deal with the dn2id index */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/dn2id.c,v 1.63.2.3 2003/03/03 17:10:09 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-ldbm.h"
#include "proto-back-ldbm.h"

int
dn2id_add(
    Backend	*be,
    struct berval *dn,
    ID		id
)
{
	int		rc, flags;
	DBCache	*db;
	Datum		key, data;
	char		*buf;
	struct berval	ptr, pdn;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "dn2id_add: (%s):%ld\n", dn->bv_val, id, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> dn2id_add( \"%s\", %ld )\n", dn->bv_val, id, 0 );
#endif

	assert( id != NOID );

	db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT );
	if ( db == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, ERR, 
			"dn2id_add: couldn't open/create dn2id%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "Could not open/create dn2id%s\n",
		    LDBM_SUFFIX, 0, 0 );
#endif

		return( -1 );
	}

	ldbm_datum_init( key );
	key.dsize = dn->bv_len + 2;
	buf = ch_malloc( key.dsize );
	key.dptr = buf;
	buf[0] = DN_BASE_PREFIX;
	ptr.bv_val = buf + 1;
	ptr.bv_len = dn->bv_len;
	AC_MEMCPY( ptr.bv_val, dn->bv_val, dn->bv_len );
	ptr.bv_val[ dn->bv_len ] = '\0';

	ldbm_datum_init( data );
	data.dptr = (char *) &id;
	data.dsize = sizeof(ID);

	flags = LDBM_INSERT;
	rc = ldbm_cache_store( db, key, data, flags );

	if ( rc != -1 && !be_issuffix( be, &ptr )) {
		buf[0] = DN_SUBTREE_PREFIX;
		ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
		rc = idl_insert_key( be, db, key, id );
		ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );

		if ( rc != -1 ) {
			dnParent( &ptr, &pdn );

			pdn.bv_val[-1] = DN_ONE_PREFIX;
			key.dsize = pdn.bv_len + 2;
			key.dptr = pdn.bv_val - 1;
			ptr = pdn;
			ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
			rc = idl_insert_key( be, db, key, id );
			ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );
		}
	}

	while ( rc != -1 && !be_issuffix( be, &ptr )) {
		ptr.bv_val[-1] = DN_SUBTREE_PREFIX;

		ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
		rc = idl_insert_key( be, db, key, id );
		ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );

		if( rc != 0 ) break;
		dnParent( &ptr, &pdn );
		key.dsize = pdn.bv_len + 2;
		key.dptr = pdn.bv_val - 1;
		ptr = pdn;
	}

	free( buf );
	ldbm_cache_close( be, db );

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "dn2id_add: return %d\n", rc, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "<= dn2id_add %d\n", rc, 0, 0 );
#endif

	return( rc );
}

int
dn2id(
    Backend	*be,
    struct berval *dn,
    ID          *idp
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	DBCache	*db;
	Datum		key, data;
	unsigned char	*tmp;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "dn2id: (%s)\n", dn->bv_val, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> dn2id( \"%s\" )\n", dn->bv_val, 0, 0 );
#endif

	assert( idp );

	/* first check the cache */
	*idp = cache_find_entry_ndn2id( be, &li->li_cache, dn );
	if ( *idp != NOID ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, DETAIL1, "dn2id: (%s)%ld in cache.\n", dn, *idp, 0 );
#else
		Debug( LDAP_DEBUG_TRACE, "<= dn2id %ld (in cache)\n", *idp,
			0, 0 );
#endif

		return( 0 );
	}

	db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT );
	if ( db == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, ERR, 
			   "dn2id: couldn't open dn2id%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "<= dn2id could not open dn2id%s\n",
			LDBM_SUFFIX, 0, 0 );
#endif
		/*
		 * return code !0 if ldbm cache open failed;
		 * callers should handle this
		 */
		*idp = NOID;
		return( -1 );
	}

	ldbm_datum_init( key );

	key.dsize = dn->bv_len + 2;
	key.dptr = ch_malloc( key.dsize );
	tmp = (unsigned char *)key.dptr;
	tmp[0] = DN_BASE_PREFIX;
	tmp++;
	AC_MEMCPY( tmp, dn->bv_val, dn->bv_len );
	tmp[dn->bv_len] = '\0';

	data = ldbm_cache_fetch( db, key );

	ldbm_cache_close( be, db );

	free( key.dptr );

	if ( data.dptr == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, INFO, "dn2id: (%s) NOID\n", dn, 0, 0 );
#else
		Debug( LDAP_DEBUG_TRACE, "<= dn2id NOID\n", 0, 0, 0 );
#endif

		*idp = NOID;
		return( 0 );
	}

	AC_MEMCPY( (char *) idp, data.dptr, sizeof(ID) );

	assert( *idp != NOID );

	ldbm_datum_free( db->dbc_db, data );

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "dn2id: %ld\n", *idp, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "<= dn2id %ld\n", *idp, 0, 0 );
#endif

	return( 0 );
}

int
dn2idl(
    Backend	*be,
    struct berval	*dn,
    int		prefix,
    ID_BLOCK    **idlp
)
{
	DBCache	*db;
	Datum		key;
	unsigned char	*tmp;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "dn2idl: \"%c%s\"\n", prefix, dn->bv_val, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> dn2idl( \"%c%s\" )\n", prefix, dn->bv_val, 0 );
#endif

	assert( idlp != NULL );
	*idlp = NULL;

	if ( prefix == DN_SUBTREE_PREFIX && be_issuffix(be, dn) ) {
		*idlp = idl_allids( be );
		return 0;
	}

	db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT );
	if ( db == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, ERR, 
			   "dn2idl: could not open dn2id%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "<= dn2idl could not open dn2id%s\n",
			LDBM_SUFFIX, 0, 0 );
#endif

		return -1;
	}

	ldbm_datum_init( key );

	key.dsize = dn->bv_len + 2;
	key.dptr = ch_malloc( key.dsize );
	tmp = (unsigned char *)key.dptr;
	tmp[0] = prefix;
	tmp++;
	AC_MEMCPY( tmp, dn->bv_val, dn->bv_len );
	tmp[dn->bv_len] = '\0';

	*idlp = idl_fetch( be, db, key );

	ldbm_cache_close( be, db );

	free( key.dptr );

	return( 0 );
}


int
dn2id_delete(
    Backend	*be,
    struct berval *dn,
	ID id
)
{
	DBCache	*db;
	Datum		key;
	int		rc;
	char		*buf;
	struct berval	ptr, pdn;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, 
		   "dn2id_delete: (%s)%ld\n", dn->bv_val, id, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> dn2id_delete( \"%s\", %ld )\n", dn->bv_val, id, 0 );
#endif


	assert( id != NOID );

	db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT );
	if ( db == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, ERR, 
			   "dn2id_delete: couldn't open db2id%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "<= dn2id_delete could not open dn2id%s\n", LDBM_SUFFIX,
		    0, 0 );
#endif

		return( -1 );
	}

	ldbm_datum_init( key );
	key.dsize = dn->bv_len + 2;
	buf = ch_malloc( key.dsize );
	key.dptr = buf;
	buf[0] = DN_BASE_PREFIX;
	ptr.bv_val = buf + 1;
	ptr.bv_len = dn->bv_len;
	AC_MEMCPY( ptr.bv_val, dn->bv_val, dn->bv_len );
	ptr.bv_val[dn->bv_len] = '\0';

	rc = ldbm_cache_delete( db, key );
	
	if( !be_issuffix( be, &ptr )) {
		buf[0] = DN_SUBTREE_PREFIX;
		ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
		(void) idl_delete_key( be, db, key, id );
		ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );

		dnParent( &ptr, &pdn );

		pdn.bv_val[-1] = DN_ONE_PREFIX;
		key.dsize = pdn.bv_len + 2;
		key.dptr = pdn.bv_val - 1;
		ptr = pdn;

		ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
		(void) idl_delete_key( be, db, key, id );
		ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );
	}

	while ( rc != -1 && !be_issuffix( be, &ptr )) {
		ptr.bv_val[-1] = DN_SUBTREE_PREFIX;

		ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
		(void) idl_delete_key( be, db, key, id );
		ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );

		dnParent( &ptr, &pdn );
		key.dsize = pdn.bv_len + 2;
		key.dptr = pdn.bv_val - 1;
		ptr = pdn;
	}

	free( buf );

	ldbm_cache_close( be, db );

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "dn2id_delete: return %d\n", rc, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "<= dn2id_delete %d\n", rc, 0, 0 );
#endif

	return( rc );
}

/*
 * dn2entry - look up dn in the cache/indexes and return the corresponding
 * entry.
 */

Entry *
dn2entry_rw(
    Backend	*be,
    struct berval *dn,
    Entry	**matched,
    int		rw
)
{
	ID		id;
	Entry		*e = NULL;
	struct berval	pdn;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, 
		   "dn2entry_rw: %s entry %s\n", rw ? "w" : "r", dn->bv_val, 0 );
#else
	Debug(LDAP_DEBUG_TRACE, "dn2entry_%s: dn: \"%s\"\n",
		rw ? "w" : "r", dn->bv_val, 0);
#endif


	if( matched != NULL ) {
		/* caller cares about match */
		*matched = NULL;
	}

	if ( dn2id( be, dn, &id ) ) {
		/* something bad happened to ldbm cache */
		return( NULL );

	}
	
	if ( id != NOID ) {
		/* try to return the entry */
		if ((e = id2entry_rw( be, id, rw )) != NULL ) {
			return( e );
		}

#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, ERR, 
			"dn2entry_rw: no entry for valid id (%ld), dn (%s)\n", 
			id, dn->bv_val, 0 );
#else
		Debug(LDAP_DEBUG_ANY,
			"dn2entry_%s: no entry for valid id (%ld), dn \"%s\"\n",
			rw ? "w" : "r", id, dn->bv_val);
#endif

		/* must have been deleted from underneath us */
		/* treat as if NOID was found */
	}

	/* caller doesn't care about match */
	if( matched == NULL ) return NULL;

	/* entry does not exist - see how much of the dn does exist */
	if ( !be_issuffix( be, dn ) && (dnParent( dn, &pdn ), pdn.bv_len) ) {
		/* get entry with reader lock */
		if ( (e = dn2entry_r( be, &pdn, matched )) != NULL ) {
			*matched = e;
		}
	}

	return NULL;
}

