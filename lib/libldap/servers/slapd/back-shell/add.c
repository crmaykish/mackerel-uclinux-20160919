/* add.c - shell backend add function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-shell/add.c,v 1.11.2.6 2003/03/03 17:10:10 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "shell.h"

int
shell_back_add(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e
)
{
	struct shellinfo	*si = (struct shellinfo *) be->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	FILE			*rfp, *wfp;
	int			len;

	if ( si->si_add == NULL ) {
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "add not implemented", NULL, NULL );
		return( -1 );
	}

	if ( ! access_allowed( be, conn, op, e,
		entry, NULL, ACL_WRITE, NULL ) )
	{
		send_ldap_result( conn, op, LDAP_INSUFFICIENT_ACCESS,
			NULL, NULL, NULL, NULL );
		return -1;
	}

	if ( (op->o_private = (void *) forkandexec( si->si_add, &rfp, &wfp )) == (void *) -1 ) {
		send_ldap_result( conn, op, LDAP_OTHER, NULL,
		    "could not fork/exec", NULL, NULL );
		return( -1 );
	}

	/* write out the request to the add process */
	fprintf( wfp, "ADD\n" );
	fprintf( wfp, "msgid: %ld\n", (long) op->o_msgid );
	print_suffixes( wfp, be );
	ldap_pvt_thread_mutex_lock( &entry2str_mutex );
	fprintf( wfp, "%s", entry2str( e, &len ) );
	ldap_pvt_thread_mutex_unlock( &entry2str_mutex );
	fclose( wfp );

	/* read in the result and send it along */
	read_and_send_results( be, conn, op, rfp, NULL, 0 );

	fclose( rfp );
	return( 0 );
}
