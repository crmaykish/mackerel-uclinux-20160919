/* unbind.c - shell backend unbind function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-shell/unbind.c,v 1.12.2.4 2003/03/03 17:10:11 kurt Exp $ */
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
shell_back_unbind(
    Backend		*be,
    Connection		*conn,
    Operation		*op
)
{
	struct shellinfo	*si = (struct shellinfo *) be->be_private;
	FILE			*rfp, *wfp;

	if ( si->si_unbind == NULL ) {
		return 0;
	}

	if ( (op->o_private = (void *) forkandexec( si->si_unbind, &rfp, &wfp ))
	    == (void *) -1 ) {
		return 0;
	}

	/* write out the request to the unbind process */
	fprintf( wfp, "UNBIND\n" );
	fprintf( wfp, "msgid: %ld\n", (long) op->o_msgid );
	print_suffixes( wfp, be );
	fprintf( wfp, "dn: %s\n", (conn->c_dn.bv_len ? conn->c_dn.bv_val : "") );
	fclose( wfp );

	/* no response to unbind */
	fclose( rfp );

	return 0;
}
