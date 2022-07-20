/* index.c - routines for dealing with attribute indexes */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/key.c,v 1.4.2.2 2003/03/03 17:10:10 kurt Exp $ */
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

/* read a key */
int
key_read(
    Backend	*be,
	DBCache *db,
    struct berval *k,
	ID_BLOCK **idout
)
{
	Datum		key;
	ID_BLOCK		*idl;

#ifdef NEW_LOGGING
	LDAP_LOG( INDEX, ENTRY, "key_read: enter\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> key_read\n", 0, 0, 0 );
#endif


	ldbm_datum_init( key );
	key.dptr = k->bv_val;
	key.dsize = k->bv_len;

	idl = idl_fetch( be, db, key );

#ifdef NEW_LOGGING
	LDAP_LOG( INDEX, ENTRY, 
		   "key_read: %ld candidates\n", idl ? ID_BLOCK_NIDS(idl) : 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "<= index_read %ld candidates\n",
	       idl ? ID_BLOCK_NIDS(idl) : 0, 0, 0 );
#endif


	*idout = idl;
	return LDAP_SUCCESS;
}

/* Add or remove stuff from index files */
int
key_change(
    Backend		*be,
    DBCache	*db,
    struct berval *k,
    ID			id,
    int			op
)
{
	int	rc;
	Datum	key;

#ifdef NEW_LOGGING
	LDAP_LOG( INDEX, ENTRY, "key_change: %s ID %lx\n",
		   op == SLAP_INDEX_ADD_OP ? "Add" : "Delete", (long)id, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> key_change(%s,%lx)\n",
		op == SLAP_INDEX_ADD_OP ? "ADD":"DELETE", (long) id, 0 );
#endif


	ldbm_datum_init( key );
	key.dptr = k->bv_val;
	key.dsize = k->bv_len;

	ldap_pvt_thread_mutex_lock( &db->dbc_write_mutex );
	if (op == SLAP_INDEX_ADD_OP) {
	    /* Add values */
	    rc = idl_insert_key( be, db, key, id );

	} else {
	    /* Delete values */
	    rc = idl_delete_key( be, db, key, id );
	}
	ldap_pvt_thread_mutex_unlock( &db->dbc_write_mutex );


#ifdef NEW_LOGGING
	LDAP_LOG( INDEX, ENTRY, "key_change: return %d\n", rc, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "<= key_change %d\n", rc, 0, 0 );
#endif


	ldap_pvt_thread_yield();

	return rc;
}
