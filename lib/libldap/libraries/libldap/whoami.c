/* $OpenLDAP: pkg/ldap/libraries/libldap/whoami.c,v 1.3.2.1 2003/02/08 23:28:51 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

/*
 * LDAP Who Am I? (Extended) Operation <draft-zeilenga-ldap-authzid-xx.txt>
 */

int ldap_parse_whoami(
	LDAP *ld,
	LDAPMessage *res,
	struct berval **authzid )
{
	int rc;
	char *retoid = NULL;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( res != NULL );
	assert( authzid != NULL );

	*authzid = NULL;

	rc = ldap_parse_extended_result( ld, res, &retoid, authzid, 0 );

	if( rc != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_parse_whoami" );
		return rc;
	}

	ber_memfree( retoid );
	return rc;
}

int
ldap_whoami( LDAP *ld,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	int				*msgidp )
{
	int rc;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msgidp != NULL );

	rc = ldap_extended_operation( ld, LDAP_EXOP_X_WHO_AM_I,
		NULL, sctrls, cctrls, msgidp );

	return rc;
}

int
ldap_whoami_s(
	LDAP *ld,
	struct berval **authzid,
	LDAPControl **sctrls,
	LDAPControl **cctrls )
{
	int		rc;
	int		msgid;
	LDAPMessage	*res;

	rc = ldap_whoami( ld, sctrls, cctrls, &msgid );
	if ( rc != LDAP_SUCCESS ) return rc;

	if ( ldap_result( ld, msgid, 1, (struct timeval *) NULL, &res ) == -1 ) {
		return ld->ld_errno;
	}

	rc = ldap_parse_whoami( ld, res, authzid );
	if( rc != LDAP_SUCCESS ) {
		ldap_msgfree( res );
		return rc;
	}

	return( ldap_result2error( ld, res, 1 ) );
}
