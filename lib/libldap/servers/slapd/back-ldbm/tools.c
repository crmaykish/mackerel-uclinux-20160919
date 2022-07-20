/* tools.c - tools for slap tools */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/tools.c,v 1.34.2.3 2003/03/03 17:10:10 kurt Exp $ */
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

static LDBMCursor *cursorp = NULL;
static DBCache *id2entry = NULL;

int ldbm_tool_entry_open(
	BackendDB *be, int mode )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	int flags;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry == NULL );

	switch( mode ) {
	case 1:
		flags = LDBM_WRCREAT;
		break;
	case 2:
#ifdef TRUNCATE_MODE
		flags = LDBM_NEWDB;
#else
		flags = LDBM_WRCREAT;
#endif
		break;
	default:
		flags = LDBM_READER;
	}

	li->li_dbwritesync = 0;

	if ( (id2entry = ldbm_cache_open( be, "id2entry", LDBM_SUFFIX, flags ))
	    == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, CRIT,
			   "Could not open/create id2entry%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "Could not open/create id2entry" LDBM_SUFFIX "\n",
		    0, 0, 0 );
#endif

		return( -1 );
	}

	return 0;
}

int ldbm_tool_entry_close(
	BackendDB *be )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	ldbm_cache_close( be, id2entry );
	li->li_dbwritesync = 1;
	id2entry = NULL;

	return 0;
}

ID ldbm_tool_entry_first(
	BackendDB *be )
{
	Datum key;
	ID id;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	key = ldbm_firstkey( id2entry->dbc_db, &cursorp );

	if( key.dptr == NULL ) {
		return NOID;
	}

	AC_MEMCPY( &id, key.dptr, key.dsize );
#ifndef WORDS_BIGENDIAN
	id = ntohl( id );
#endif

	ldbm_datum_free( id2entry->dbc_db, key );

	return id;
}

ID ldbm_tool_entry_next(
	BackendDB *be )
{
	Datum key;
	ID id;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	/* allow for NEXTID */
	ldbm_datum_init( key );

	key = ldbm_nextkey( id2entry->dbc_db, key, cursorp );

	if( key.dptr == NULL ) {
		return NOID;
	}

	AC_MEMCPY( &id, key.dptr, key.dsize );
#ifndef WORDS_BIGENDIAN
	id = ntohl( id );
#endif

	ldbm_datum_free( id2entry->dbc_db, key );

	return id;
}

Entry* ldbm_tool_entry_get( BackendDB *be, ID id )
{
	Entry *e;
	Datum key, data;
#ifndef WORDS_BIGENDIAN
	ID id2;
#endif
	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	ldbm_datum_init( key );

#ifndef WORDS_BIGENDIAN
	id2 = htonl( id );
	key.dptr = (char *) &id2;
#else
	key.dptr = (char *) &id;
#endif
	key.dsize = sizeof(ID);

	data = ldbm_cache_fetch( id2entry, key );

	if ( data.dptr == NULL ) {
		return NULL;
	}

	e = str2entry( data.dptr );
	ldbm_datum_free( id2entry->dbc_db, data );

	if( e != NULL ) {
		e->e_id = id;
	}

	return e;
}

ID ldbm_tool_entry_put(
	BackendDB *be,
	Entry *e,
	struct berval *text )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	Datum key, data;
	int rc, len;
	ID id;

	assert( slapMode & SLAP_TOOL_MODE );
	assert( id2entry != NULL );

	assert( text );
	assert( text->bv_val );
	assert( text->bv_val[0] == '\0' );	/* overconservative? */

	if ( next_id_get( be, &id ) || id == NOID ) {
		strncpy( text->bv_val, "unable to get nextid", text->bv_len );
		return NOID;
	}

	e->e_id = li->li_nextid++;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY,
		"ldbm_tool_entry_put: (%s)%ld\n", e->e_dn, e->e_id ,0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> ldbm_tool_entry_put( %ld, \"%s\" )\n",
		e->e_id, e->e_dn, 0 );
#endif

	if ( dn2id( be, &e->e_nname, &id ) ) {
		/* something bad happened to ldbm cache */
		strncpy( text->bv_val, "ldbm cache corrupted", text->bv_len );
		return NOID;
	}

	if( id != NOID ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, ENTRY,
			"ldbm_tool_entry_put: \"%s\" already exists (id=%ld)\n",
			e->e_dn, id, 0 );
#else
		Debug( LDAP_DEBUG_TRACE,
			"<= ldbm_tool_entry_put: \"%s\" already exists (id=%ld)\n",
			e->e_ndn, id, 0 );
#endif
		strncpy( text->bv_val, "already exists", text->bv_len );
		return NOID;
	}

	rc = index_entry_add( be, e, e->e_attrs );
	if( rc != 0 ) {
		strncpy( text->bv_val, "index add failed", text->bv_len );
		return NOID;
	}

	rc = dn2id_add( be, &e->e_nname, e->e_id );
	if( rc != 0 ) {
		strncpy( text->bv_val, "dn2id add failed", text->bv_len );
		return NOID;
	}

	ldbm_datum_init( key );
	ldbm_datum_init( data );

#ifndef WORDS_BIGENDIAN
	id = htonl( e->e_id );
	key.dptr = (char *) &id;
#else
	key.dptr = (char *) &e->e_id;
#endif
	key.dsize = sizeof(ID);

	data.dptr = entry2str( e, &len );
	data.dsize = len + 1;

	/* store it */
	rc = ldbm_cache_store( id2entry, key, data, LDBM_REPLACE );

	if( rc != 0 ) {
		(void) dn2id_delete( be, &e->e_nname, e->e_id );
		strncpy( text->bv_val, "cache store failed", text->bv_len );
		return NOID;
	}

	return e->e_id;
}

int ldbm_tool_entry_reindex(
	BackendDB *be,
	ID id )
{
	int rc;
	Entry *e;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "ldbm_tool_entry_reindex: ID=%ld\n", 
		(long)id, 0, 0 );
#else
	Debug( LDAP_DEBUG_ARGS, "=> ldbm_tool_entry_reindex( %ld )\n",
		(long) id, 0, 0 );
#endif


	e = ldbm_tool_entry_get( be, id );

	if( e == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, INFO,
		   "ldbm_tool_entry_reindex: could not locate id %ld\n", 
		   (long)id, 0, 0  );
#else
		Debug( LDAP_DEBUG_ANY,
			"ldbm_tool_entry_reindex:: could not locate id=%ld\n",
			(long) id, 0, 0 );
#endif

		return -1;
	}

	/*
	 * just (re)add them for now
	 * assume that some other routine (not yet implemented)
	 * will zap index databases
	 *
	 */

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY,
		   "ldbm_tool_entry_reindex: (%s) %ld\n", e->e_dn, id, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> ldbm_tool_entry_reindex( %ld, \"%s\" )\n",
		id, e->e_dn, 0 );
#endif

	dn2id_add( be, &e->e_nname, e->e_id );
	rc = index_entry_add( be, e, e->e_attrs );

	entry_free( e );

	return rc;
}

int ldbm_tool_sync( BackendDB *be )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;

	assert( slapMode & SLAP_TOOL_MODE );

	if ( li->li_nextid != NOID ) {
		if ( next_id_write( be, li->li_nextid ) ) {
			return( -1 );
		}
	}

	return 0;
}
