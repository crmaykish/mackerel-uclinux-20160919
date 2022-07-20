/* config.c - DNS SRV backend configuration file routine */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-dnssrv/config.c,v 1.6.2.2 2003/03/03 17:10:09 kurt Exp $ */
/*
 * Copyright 2000-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "external.h"

int
dnssrv_back_db_config(
    BackendDB	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv )
{
	struct ldapinfo	*li = (struct ldapinfo *) be->be_private;

	if ( li == NULL ) {
		fprintf( stderr, "%s: line %d: DNSSRV backend info is null!\n",
		    fname, lineno );
		return( 1 );
	}

	/* no configuration options (yet) */
	{
		fprintf( stderr,
			"%s: line %d: unknown directive \"%s\""
			" in DNSSRV database definition (ignored)\n",
		    fname, lineno, argv[0] );
	}
	return 0;
}
