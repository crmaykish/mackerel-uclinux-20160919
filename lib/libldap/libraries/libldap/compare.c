/* $OpenLDAP: pkg/ldap/libraries/libldap/compare.c,v 1.16.2.4 2003/02/09 17:02:18 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*  Portions
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  compare.c
 */

/* The compare request looks like this:
 *	CompareRequest ::= SEQUENCE {
 *		entry	DistinguishedName,
 *		ava	SEQUENCE {
 *			type	AttributeType,
 *			value	AttributeValue
 *		}
 *	}
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"
#include "ldap_log.h"

/*
 * ldap_compare_ext - perform an ldap extended compare operation.  The dn
 * of the entry to compare to and the attribute and value to compare (in
 * attr and value) are supplied.  The msgid of the response is returned.
 *
 * Example:
 *	struct berval bvalue = { "secret", sizeof("secret")-1 };
 *	rc = ldap_compare( ld, "c=us@cn=bob",
 *		"userPassword", &bvalue,
 *		sctrl, cctrl, &msgid )
 */
int
ldap_compare_ext(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *attr,
	struct berval *bvalue,
	LDAPControl **sctrls,
	LDAPControl **cctrls,
	int	*msgidp )
{
	int rc;
	BerElement	*ber;

#ifdef NEW_LOGGING
	LDAP_LOG ( OPERATION, ENTRY, "ldap_compare\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_compare\n", 0, 0, 0 );
#endif

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( dn != NULL );
	assert( attr != NULL );
	assert( msgidp != NULL );

	/* check client controls */
	rc = ldap_int_client_controls( ld, cctrls );
	if( rc != LDAP_SUCCESS ) return rc;

	/* create a message to send */
	if ( (ber = ldap_alloc_ber_with_options( ld )) == NULL ) {
		return( LDAP_NO_MEMORY );
	}

	if ( ber_printf( ber, "{it{s{sON}N}", /* '}' */
		++ld->ld_msgid,
		LDAP_REQ_COMPARE, dn, attr, bvalue ) == -1 )
	{
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( ld->ld_errno );
	}

	/* Put Server Controls */
	if( ldap_int_put_controls( ld, sctrls, ber ) != LDAP_SUCCESS ) {
		ber_free( ber, 1 );
		return ld->ld_errno;
	}

	if( ber_printf( ber, /*{*/ "N}" ) == -1 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( ld->ld_errno );
	}


	/* send the message */
	*msgidp = ldap_send_initial_request( ld, LDAP_REQ_COMPARE, dn, ber );
	return ( *msgidp < 0 ? ld->ld_errno : LDAP_SUCCESS );
}

/*
 * ldap_compare_ext - perform an ldap extended compare operation.  The dn
 * of the entry to compare to and the attribute and value to compare (in
 * attr and value) are supplied.  The msgid of the response is returned.
 *
 * Example:
 *	msgid = ldap_compare( ld, "c=us@cn=bob", "userPassword", "secret" )
 */
int
ldap_compare(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *attr,
	LDAP_CONST char *value )
{
	int msgid;
	struct berval bvalue;

	assert( value != NULL );

	bvalue.bv_val = (char *) value;
	bvalue.bv_len = (value == NULL) ? 0 : strlen( value );

	return ldap_compare_ext( ld, dn, attr, &bvalue, NULL, NULL, &msgid ) == LDAP_SUCCESS
		? msgid : -1;
}

int
ldap_compare_ext_s(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *attr,
	struct berval *bvalue,
	LDAPControl **sctrl,
	LDAPControl **cctrl )
{
	int		rc;
	int		msgid;
	LDAPMessage	*res;

	rc = ldap_compare_ext( ld, dn, attr, bvalue, sctrl, cctrl, &msgid );

	if (  rc != LDAP_SUCCESS )
		return( rc );

	if ( ldap_result( ld, msgid, 1, (struct timeval *) NULL, &res ) == -1 )
		return( ld->ld_errno );

	return( ldap_result2error( ld, res, 1 ) );
}

int
ldap_compare_s(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *attr,
	LDAP_CONST char *value )
{
	struct berval bvalue;

	assert( value != NULL );

	bvalue.bv_val = (char *) value;
	bvalue.bv_len = (value == NULL) ? 0 : strlen( value );

	return ldap_compare_ext_s( ld, dn, attr, &bvalue, NULL, NULL );
}
