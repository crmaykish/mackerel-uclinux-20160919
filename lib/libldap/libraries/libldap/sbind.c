/* $OpenLDAP: pkg/ldap/libraries/libldap/sbind.c,v 1.16.2.3 2003/03/03 17:10:05 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*  Portions
 *  Copyright (c) 1993 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  sbind.c
 */

/*
 *	BindRequest ::= SEQUENCE {
 *		version		INTEGER,
 *		name		DistinguishedName,	 -- who
 *		authentication	CHOICE {
 *			simple		[0] OCTET STRING -- passwd
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
 *			krbv42ldap	[1] OCTET STRING
 *			krbv42dsa	[2] OCTET STRING
#endif
 *			sasl		[3] SaslCredentials	-- LDAPv3
 *		}
 *	}
 *
 *	BindResponse ::= SEQUENCE {
 *		COMPONENTS OF LDAPResult,
 *		serverSaslCreds		OCTET STRING OPTIONAL -- LDAPv3
 *	}
 *
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

/*
 * ldap_simple_bind - bind to the ldap server (and X.500).  The dn and
 * password of the entry to which to bind are supplied.  The message id
 * of the request initiated is returned.
 *
 * Example:
 *	ldap_simple_bind( ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret" )
 */

int
ldap_simple_bind(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *passwd )
{
	int rc;
	int msgid;
	struct berval cred;

#ifdef NEW_LOGGING
	LDAP_LOG ( OPERATION, ENTRY, "ldap_simple_bind\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_simple_bind\n", 0, 0, 0 );
#endif

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );

	if ( passwd != NULL ) {
		cred.bv_val = (char *) passwd;
		cred.bv_len = strlen( passwd );
	} else {
		cred.bv_val = "";
		cred.bv_len = 0;
	}

	rc = ldap_sasl_bind( ld, dn, LDAP_SASL_SIMPLE, &cred,
		NULL, NULL, &msgid );

	return rc == LDAP_SUCCESS ? msgid : -1;
}

/*
 * ldap_simple_bind - bind to the ldap server (and X.500) using simple
 * authentication.  The dn and password of the entry to which to bind are
 * supplied.  LDAP_SUCCESS is returned upon success, the ldap error code
 * otherwise.
 *
 * Example:
 *	ldap_simple_bind_s( ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret" )
 */

int
ldap_simple_bind_s( LDAP *ld, LDAP_CONST char *dn, LDAP_CONST char *passwd )
{
	struct berval cred;

#ifdef NEW_LOGGING
	LDAP_LOG ( OPERATION, ENTRY, "ldap_simple_bind_s\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_simple_bind_s\n", 0, 0, 0 );
#endif

	if ( passwd != NULL ) {
		cred.bv_val = (char *) passwd;
		cred.bv_len = strlen( passwd );
	} else {
		cred.bv_val = "";
		cred.bv_len = 0;
	}

	return ldap_sasl_bind_s( ld, dn, LDAP_SASL_SIMPLE, &cred,
		NULL, NULL, NULL );
}
