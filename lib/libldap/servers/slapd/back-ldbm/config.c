/* config.c - ldbm backend configuration file routine */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/config.c,v 1.29.2.4 2003/03/03 17:10:09 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "back-ldbm.h"

int
ldbm_back_db_config(
    Backend	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	int rc;
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;

	if ( li == NULL ) {
		fprintf( stderr, "%s: line %d: ldbm database info is null!\n",
		    fname, lineno );
		return( 1 );
	}

	/* directory where database files live */
	if ( strcasecmp( argv[0], "directory" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
		"%s: line %d: missing dir in \"directory <dir>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		if ( li->li_directory )
			free( li->li_directory );
		li->li_directory = ch_strdup( argv[1] );

	/* mode with which to create new database files */
	} else if ( strcasecmp( argv[0], "mode" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
			"%s: line %d: missing mode in \"mode <mode>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		li->li_mode = strtol( argv[1], NULL, 0 );

	/* attribute to index */
	} else if ( strcasecmp( argv[0], "index" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
"%s: line %d: missing attr in \"index <attr> [pres,eq,approx,sub]\" line\n",
			    fname, lineno );
			return( 1 );
		} else if ( argc > 3 ) {
			fprintf( stderr,
"%s: line %d: extra junk after \"index <attr> [pres,eq,approx,sub]\" line (ignored)\n",
			    fname, lineno );
		}
		rc = attr_index_config( li, fname, lineno, argc - 1, &argv[1] );

		if( rc != LDAP_SUCCESS ) return 1;

	/* size of the cache in entries */
	} else if ( strcasecmp( argv[0], "cachesize" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
		"%s: line %d: missing size in \"cachesize <size>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		li->li_cache.c_maxsize = atoi( argv[1] );

	/* size of each dbcache in bytes */
	} else if ( strcasecmp( argv[0], "dbcachesize" ) == 0 ) {
		if ( argc < 2 ) {
			fprintf( stderr,
		"%s: line %d: missing size in \"dbcachesize <size>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		li->li_dbcachesize = atoi( argv[1] );

	/* no locking (not safe) */
	} else if ( strcasecmp( argv[0], "dbnolocking" ) == 0 ) {
		li->li_dblocking = 0;

	/* no write sync (not safe) */
	} else if ( ( strcasecmp( argv[0], "dbnosync" ) == 0 )
		|| ( strcasecmp( argv[0], "dbcachenowsync" ) == 0 ) )
	{
		li->li_dbwritesync = 0;

	/* run sync thread */
	} else if ( strcasecmp( argv[0], "dbsync" ) == 0 ) {
#ifndef NO_THREADS
		int i;
		if ( argc < 2 ) {
#ifdef NEW_LOGGING
			LDAP_LOG ( CONFIG, ERR, "ldbm_back_db_config: %s: "
				"line %d: missing frequency value in \"dbsync <frequency> "
				"[<wait-times> [wait-interval]]\" line\n", fname, lineno, 0 );
#else	
			Debug( LDAP_DEBUG_ANY,
    "%s: line %d: missing frquency value in \"dbsync <frequency> [<wait-times> [wait-interval]]\" line\n",
			    fname, lineno, 0 );
#endif
			return 1;
		}

		i = atoi( argv[1] );

		if( i < 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG ( CONFIG, ERR, 
				"ldbm_back_db_config: %s: "
				"line %d: frequency value (%d) invalid \"dbsync "
				"<frequency> [<wait-times> [wait-interval]]\" line\n", 
				fname, lineno, i );
#else	
			Debug( LDAP_DEBUG_ANY,
    "%s: line %d: frquency value (%d) invalid \"dbsync <frequency> [<wait-times> [wait-interval]]\" line\n",
			    fname, lineno, i );
#endif
			return 1;
		}

		li->li_dbsyncfreq = i;

		if ( argc > 2 ) {
			i = atoi( argv[2] );
			if ( i < 0 ) {
#ifdef NEW_LOGGING
				LDAP_LOG ( CONFIG,ERR, "ldbm_back_db_config: %s: "
					"line %d: frequency value (%d) invalid \"dbsync "
					"<frequency> [<wait-times> [wait-interval]]\" line\n", 
					fname, lineno, i );
#else	
				Debug( LDAP_DEBUG_ANY,
	    "%s: line %d: frquency value (%d) invalid \"dbsync <frequency> [<wait-times> [wait-interval]]\" line\n",
				    fname, lineno, i );
#endif
				return 1;
			}
			li ->li_dbsyncwaitn = i;
		}

		if ( argc > 3 ) {
			i = atoi( argv[3] );
			if ( i <= 0 ) {
#ifdef NEW_LOGGING
				LDAP_LOG ( CONFIG,ERR, "ldbm_back_db_config: %s: "
					"line %d: frequency value (%d) invalid \"dbsync "
					"<frequency> [<wait-times> [wait-interval]]\" line\n", 
					fname, lineno, i );
#else	
				Debug( LDAP_DEBUG_ANY,
	    "%s: line %d: frquency value (%d) invalid \"dbsync <frequency> [<wait-times> [wait-interval]]\" line\n",
				    fname, lineno, i );
#endif
				return 1;
			}
			li ->li_dbsyncwaitinterval = i;
		}

		/* turn off writesync when sync policy is in place */
		li->li_dbwritesync = 0;

#else
#ifdef NEW_LOGGING
		LDAP_LOG ( CONFIG, ERR, "ldbm_back_db_config: \"dbsync\""
			" policies not supported in non-threaded environments\n", 0, 0, 0 );
#else	
		Debug( LDAP_DEBUG_ANY,
    "\"dbsync\" policies not supported in non-threaded environments\n", 0, 0, 0);
#endif
		return 1;
#endif


	/* anything else */
	} else {
		fprintf( stderr,
"%s: line %d: unknown directive \"%s\" in ldbm database definition (ignored)\n",
		    fname, lineno, argv[0] );
	}

	return 0;
}
