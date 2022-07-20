/* lock.c - routines to open and apply an advisory lock to a file */
/* $OpenLDAP: pkg/ldap/servers/slapd/lock.c,v 1.20.2.5 2003/03/12 19:19:21 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/time.h>
#include <ac/unistd.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "slap.h"
#include <lutil.h>

FILE *
lock_fopen( const char *fname, const char *type, FILE **lfp )
{
	FILE	*fp;
	char	buf[MAXPATHLEN];

	/* open the lock file */
	snprintf( buf, sizeof buf, "%s.lock", fname );

	if ( (*lfp = fopen( buf, "w" )) == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"lock_fopen: could not open lock file \"%s\".\n", buf, 0, 0);
#else
		Debug( LDAP_DEBUG_ANY, "could not open \"%s\"\n", buf, 0, 0 );
#endif

		return( NULL );
	}

	/* acquire the lock */
	ldap_lockf( fileno(*lfp) );

	/* open the log file */
	if ( (fp = fopen( fname, type )) == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"lock_fopen: could not open log file \"%s\".\n", buf, 0, 0);
#else
		Debug( LDAP_DEBUG_ANY, "could not open \"%s\"\n", fname, 0, 0 );
#endif

		ldap_unlockf( fileno(*lfp) );
		fclose( *lfp );
		*lfp = NULL;
		return( NULL );
	}

	return( fp );
}

int
lock_fclose( FILE *fp, FILE *lfp )
{
	/* unlock */
	ldap_unlockf( fileno(lfp) );
	fclose( lfp );

	return( fclose( fp ) );
}
