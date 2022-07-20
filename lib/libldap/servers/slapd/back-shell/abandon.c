/* abandon.c - shell backend abandon function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-shell/abandon.c,v 1.13.2.6 2003/05/23 10:45:18 hallvard Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "shell.h"

int
shell_back_abandon(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    int		msgid
)
{
	struct shellinfo	*si = (struct shellinfo *) be->be_private;
	FILE			*rfp, *wfp;
	pid_t			pid;
	Operation		*o;

	if ( si->si_abandon == NULL ) {
		return 0;
	}

	pid = -1;
	LDAP_STAILQ_FOREACH( o, &conn->c_ops, o_next ) {
		if ( o->o_msgid == msgid ) {
			pid = (pid_t) o->o_private;
			break;
		}
	}

	if ( pid == -1 ) {
		Debug( LDAP_DEBUG_ARGS, "shell could not find op %d\n", msgid, 0, 0 );
		return 0;
	}

	if ( forkandexec( si->si_abandon, &rfp, &wfp ) == -1 ) {
		return 0;
	}

	/* write out the request to the abandon process */
	fprintf( wfp, "ABANDON\n" );
	fprintf( wfp, "msgid: %d\n", msgid );
	print_suffixes( wfp, be );
	fprintf( wfp, "pid: %ld\n", (long) pid );
	fclose( wfp );

	/* no result from abandon */
	fclose( rfp );

	return 0;
}
