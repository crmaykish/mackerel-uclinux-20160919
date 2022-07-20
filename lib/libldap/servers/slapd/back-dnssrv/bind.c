/* bind.c - DNS SRV backend bind function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-dnssrv/bind.c,v 1.11.2.2 2003/03/03 17:10:09 kurt Exp $ */
/*
 * Copyright 2000-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "external.h"

int
dnssrv_back_bind(
    Backend		*be,
    Connection		*conn,
    Operation		*op,
    struct berval	*dn,
    struct berval	*ndn,
    int			method,
    struct berval	*cred,
    struct berval	*edn )
{
	Debug( LDAP_DEBUG_TRACE, "DNSSRV: bind %s (%d)\n",
		dn->bv_val == NULL ? "" : dn->bv_val, 
		method, NULL );
		
	if( method == LDAP_AUTH_SIMPLE && cred != NULL && cred->bv_len ) {
		Statslog( LDAP_DEBUG_STATS,
		   	"conn=%lu op=%lu DNSSRV BIND dn=\"%s\" provided passwd\n",
	   		 op->o_connid, op->o_opid,
			dn->bv_val == NULL ? "" : dn->bv_val , 0, 0 );

		Debug( LDAP_DEBUG_TRACE,
			"DNSSRV: BIND dn=\"%s\" provided cleartext password\n",
			dn->bv_val == NULL ? "" : dn->bv_val, 0, 0 );

		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM,
			NULL, "you shouldn\'t send strangers your password",
			NULL, NULL );

	} else {
		Debug( LDAP_DEBUG_TRACE, "DNSSRV: BIND dn=\"%s\"\n",
			dn->bv_val == NULL ? "" : dn->bv_val, 0, 0 );

		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM,
			NULL, "anonymous bind expected",
			NULL, NULL );
	}

	return 1;
}
