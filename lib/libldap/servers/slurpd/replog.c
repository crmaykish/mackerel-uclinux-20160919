/* $OpenLDAP: pkg/ldap/servers/slurpd/replog.c,v 1.10.2.5 2003/03/03 17:10:11 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */


/*
 * replog.c - routines which read and write replication log files.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/errno.h>
#include <ac/param.h>
#include <ac/string.h>
#include <ac/syslog.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <sys/stat.h>

#include <fcntl.h>

#include "slurp.h"
#include "globals.h"

/*
 * Copy the replication log.  Returns 0 on success, 1 if a temporary
 * error occurs, and -1 if a fatal error occurs.
 */
int
copy_replog(
    char	*src,
    char	*dst
)
{
    int		rc = 0;
    FILE	*rfp;	/* replog fp */
    FILE	*lfp;	/* replog lockfile fp */
    FILE	*dfp;	/* duplicate replog fp */
    FILE	*dlfp;	/* duplicate replog lockfile fp */
    static char	buf[ MAXPATHLEN ];
    static char	rbuf[ 1024 ];
    char	*p;

#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ARGS, "copy_replog: "
		"copy replog \"%s\" to \"%s\"\n", src, dst, 0 );
#else
    Debug( LDAP_DEBUG_ARGS,
	    "copy replog \"%s\" to \"%s\"\n", 
	    src, dst, 0 );
#endif

    /*
     * Make sure the destination directory is writable.  If not, exit
     * with a fatal error.
     */
    strcpy( buf, src );
    if (( p = strrchr( buf, LDAP_DIRSEP[0] )) == NULL ) {
	strcpy( buf, "." );
    } else {
	*p = '\0';
    }
    if ( access( buf, W_OK ) < 0 ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "copy_replog: "
		"Error: (%ld): Directory %s is not writable\n",
		(long) getpid(), buf, 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"Error: copy_replog (%ld): Directory %s is not writable\n",
		(long) getpid(), buf, 0 );
#endif
	return( -1 );
    }
    strcpy( buf, dst );
    if (( p = strrchr( buf, LDAP_DIRSEP[0] )) == NULL ) {
	strcpy( buf, "." );
    } else {
	*p = '\0';
    }
    if ( access( buf, W_OK ) < 0 ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "copy_replog: "
		"Error: (%ld): Directory %s is not writable\n",
		(long) getpid(), buf, 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"Error: copy_replog (%ld): Directory %s is not writable\n",
		(long) getpid(), buf, 0 );
#endif
	return( -1 );
    }

    /* lock src */
    rfp = lock_fopen( src, "r", &lfp );
    if ( rfp == NULL ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "copy_replog: "
		"Error: Can't lock replog \"%s\" for read: %s\n",
		src, sys_errlist[ errno ], 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"Error: copy_replog: Can't lock replog \"%s\" for read: %s\n",
		src, sys_errlist[ errno ], 0 );
#endif
	return( 1 );
    }

    /* lock dst */
    dfp = lock_fopen( dst, "a", &dlfp );
    if ( dfp == NULL ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "copy_replog: "
		"Error: Can't lock replog \"%s\" for write: %s\n",
		src, sys_errlist[ errno ], 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"Error: copy_replog: Can't lock replog \"%s\" for write: %s\n",
		src, sys_errlist[ errno ], 0 );
#endif
	lock_fclose( rfp, lfp );
	return( 1 );
    }

    /*
     * Make our own private copy of the replication log.
     */
    while (( p = fgets( rbuf, sizeof( rbuf ), rfp )) != NULL ) {
	fputs( rbuf, dfp );
    }
    /* Only truncate the source file if we're not in one-shot mode */
    if ( !sglob->one_shot_mode ) {
	/* truncate replication log */
	truncate( src, (off_t) 0 );
    }

    if ( lock_fclose( dfp, dlfp ) == EOF ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "copy_replog: "
		"Error: Error closing \"%s\"\n", src, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"Error: copy_replog: Error closing \"%s\"\n",
		src, 0, 0 );
#endif
    }
    if ( lock_fclose( rfp, lfp ) == EOF ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "copy_replog: "
		"Error: Error closing \"%s\"\n", src, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY,
		"Error: copy_replog: Error closing \"%s\"\n",
		src, 0, 0 );
#endif
    }
    return( rc );
}




/*
 * Return 1 if the given file exists and has a nonzero size,
 * 0 if it is empty or nonexistent.
 */
int
file_nonempty(
    char	*filename
)
{
    static struct stat 	stbuf;

    if ( stat( filename, &stbuf ) < 0 ) {
	return( 0 );
    } else {
	return( stbuf.st_size > (off_t ) 0 );
    }
}
