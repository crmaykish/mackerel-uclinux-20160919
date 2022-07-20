/* $OpenLDAP: pkg/ldap/servers/slapd/user.c,v 1.17.2.2 2003/03/03 17:10:07 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* user.c - set user id, group id and group access list
 *
 * Copyright 1999 by PM Lashley.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted only
 * as authorized by the OpenLDAP Public License.  A copy of this
 * license is available at http://www.OpenLDAP.org/license.html or
 * in file LICENSE in the top-level directory of the distribution.
*/

#include "portable.h"

#if defined(HAVE_SETUID) && defined(HAVE_SETGID)

#include <stdio.h>

#include <ac/stdlib.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#include <ac/ctype.h>
#include <ac/unistd.h>

#include "slap.h"

/*
 * Set real and effective user id and group id, and group access list
 * The user and group arguments are freed.
 */

void
slap_init_user( char *user, char *group )
{
    uid_t	uid = 0;
    gid_t	gid = 0;
    int		got_uid = 0, got_gid = 0;

    if ( user ) {
	struct passwd *pwd;
	if ( isdigit( (unsigned char) *user )) {
	    got_uid = 1;
	    uid = atoi( user );
#ifdef HAVE_GETPWUID
	    pwd = getpwuid( uid );
	    goto did_getpw;
#else
	    free( user );
	    user = NULL;
#endif
	} else {
	    pwd = getpwnam( user );
	did_getpw:
	    if ( pwd == NULL ) {
#ifdef NEW_LOGGING
		    LDAP_LOG( OPERATION, INFO, 
				"slap_init_user: No passwd entry for user %s\n", user, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "No passwd entry for user %s\n",
		       user, 0, 0 );
#endif

		exit( EXIT_FAILURE );
	    }
	    if ( got_uid ) {
		free( user );
		user = (pwd != NULL ? ch_strdup( pwd->pw_name ) : NULL);
	    } else {
		got_uid = 1;
		uid = pwd->pw_uid;
	    }
	    got_gid = 1;
	    gid = pwd->pw_gid;
#ifdef HAVE_ENDPWENT
	    endpwent();
#endif
	}
    }

    if ( group ) {
	struct group *grp;
	if ( isdigit( (unsigned char) *group )) {
	    gid = atoi( group );
#ifdef HAVE_GETGRGID
	    grp = getgrgid( gid );
	    goto did_group;
#endif
	} else {
	    grp = getgrnam( group );
	    if ( grp != NULL )
		gid = grp->gr_gid;
	did_group:
	    if ( grp == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			"slap_init_user: No group entry for group %s\n", group, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "No group entry for group %s\n",
		       group, 0, 0 );
#endif

		exit( EXIT_FAILURE );
	    }
	}
	free( group );
	got_gid = 1;
    }

    if ( user ) {
	if ( getuid() == 0 && initgroups( user, gid ) != 0 ) {
#ifdef NEW_LOGGING
	    LDAP_LOG( OPERATION, INFO,
			"slap_init_user: Could not set the group access (gid) list.\n", 
			0, 0, 0 );
#else
	    Debug( LDAP_DEBUG_ANY,
		   "Could not set the group access (gid) list\n", 0, 0, 0 );
#endif

	    exit( EXIT_FAILURE );
	}
	free( user );
    }

#ifdef HAVE_ENDGRENT
    endgrent();
#endif

    if ( got_gid ) {
	if ( setgid( gid ) != 0 ) {
#ifdef NEW_LOGGING
	    LDAP_LOG( OPERATION, INFO, 
			"slap_init_user: could not set real group id to %d\n", 
			(int)gid, 0, 0);
#else
	    Debug( LDAP_DEBUG_ANY, "Could not set real group id to %d\n",
		       (int) gid, 0, 0 );
#endif

	    exit( EXIT_FAILURE );
	}
#ifdef HAVE_SETEGID
	if ( setegid( gid ) != 0 ) {
#ifdef NEW_LOGGING
	    LDAP_LOG( OPERATION, INFO, 
		   "slap_init_user: Could not set effective group id to %d\n",
		   (int)gid, 0, 0);
#else
	    Debug( LDAP_DEBUG_ANY, "Could not set effective group id to %d\n",
		       (int) gid, 0, 0 );
#endif

	    exit( EXIT_FAILURE );
	}
#endif
    }

    if ( got_uid ) {
	if ( setuid( uid ) != 0 ) {
#ifdef NEW_LOGGING
	    LDAP_LOG( OPERATION, INFO, 
			"slap_init_user: Could not set real user id to %d\n", 
			(int)uid, 0, 0 );
#else
	    Debug( LDAP_DEBUG_ANY, "Could not set real user id to %d\n",
		       (int) uid, 0, 0 );
#endif

	    exit( EXIT_FAILURE );
	}
#ifdef HAVE_SETEUID
	if ( seteuid( uid ) != 0 ) {
#ifdef NEW_LOGGING
	    LDAP_LOG( OPERATION, INFO, 
			"slap_init_user: Could not set effective user id to %d\n", 
			(int)uid, 0, 0 );
#else
	    Debug( LDAP_DEBUG_ANY, "Could not set effective user id to %d\n",
		       (int) uid, 0, 0 );
#endif

	    exit( EXIT_FAILURE );
	}
#endif
    }
}

#endif /* HAVE_PWD_H && HAVE_GRP_H */
