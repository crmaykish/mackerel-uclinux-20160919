/* close.c - close ldbm backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/close.c,v 1.14.2.2 2003/03/03 17:10:09 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"
#include "back-ldbm.h"

int
ldbm_back_db_close( Backend *be )
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	if ( li->li_dbsyncfreq > 0 )
	{
		li->li_dbshutdown++;
		ldap_pvt_thread_join( li->li_dbsynctid, (void *) NULL );
	}
#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, CRIT,
		   "ldbm_back_db_close: ldbm backend syncing\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldbm backend syncing\n", 0, 0, 0 );
#endif

	ldbm_cache_flush_all( be );
#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, CRIT,
		   "ldbm_back_db_close: ldbm backend synch'ed\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldbm backend done syncing\n", 0, 0, 0 );
#endif


	cache_release_all( &((struct ldbminfo *) be->be_private)->li_cache );

	return 0;
}
