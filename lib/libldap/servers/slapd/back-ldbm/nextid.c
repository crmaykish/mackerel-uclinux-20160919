/* nextid.c - keep track of the next id to be given out */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/nextid.c,v 1.31.2.2 2003/03/03 17:10:10 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/param.h>

#include "slap.h"
#include "back-ldbm.h"

static int
next_id_read( Backend *be, ID *idp )
{
	Datum key, data;
	DBCache *db;

	*idp = NOID;

	if ( (db = ldbm_cache_open( be, "nextid", LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, CRIT,
		   "next_id_read: could not open/create nextid%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "Could not open/create nextid" LDBM_SUFFIX "\n",
			0, 0, 0 );
#endif

		return( -1 );
	}

	ldbm_datum_init( key );
	key.dptr = (char *) idp;
	key.dsize = sizeof(ID);

	data = ldbm_cache_fetch( db, key );

	if( data.dptr != NULL ) {
		AC_MEMCPY( idp, data.dptr, sizeof( ID ) );
		ldbm_datum_free( db->dbc_db, data );

	} else {
		*idp = 1;
	}

	ldbm_cache_close( be, db );
	return( 0 );
}

int
next_id_write( Backend *be, ID id )
{
	Datum key, data;
	DBCache *db;
	ID noid = NOID;
	int flags, rc = 0;

	if ( (db = ldbm_cache_open( be, "nextid", LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, CRIT,
		  "next_id_write: Could not open/create nextid%s\n", LDBM_SUFFIX, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "Could not open/create nextid" LDBM_SUFFIX "\n",
		    0, 0, 0 );
#endif

		return( -1 );
	}

	ldbm_datum_init( key );
	ldbm_datum_init( data );

	key.dptr = (char *) &noid;
	key.dsize = sizeof(ID);

	data.dptr = (char *) &id;
	data.dsize = sizeof(ID);

	flags = LDBM_REPLACE;
	if ( ldbm_cache_store( db, key, data, flags ) != 0 ) {
		rc = -1;
	}

	ldbm_cache_close( be, db );
	return( rc );
}

int
next_id_get( Backend *be, ID *idp )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	int rc = 0;

	*idp = NOID;

	if ( li->li_nextid == NOID ) {
		if ( ( rc = next_id_read( be, idp ) ) ) {
			return( rc );
		}
		li->li_nextid = *idp;
	}

	*idp = li->li_nextid;

	return( rc );
}

int
next_id( Backend *be, ID *idp )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	int rc = 0;

	if ( li->li_nextid == NOID ) {
		if ( ( rc = next_id_read( be, idp ) ) ) {
			return( rc );
		}
		li->li_nextid = *idp;
	}

	*idp = li->li_nextid++;
	if ( next_id_write( be, li->li_nextid ) ) {
		rc = -1;
	}

	return( rc );
}
