/* $OpenLDAP: pkg/ldap/libraries/libldap/passwd.c,v 1.6.2.2 2003/02/17 16:49:53 kurt Exp $ */
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
 * LDAP Password Modify (Extended) Operation <RFC 3???>
 */

int ldap_parse_passwd(
	LDAP *ld,
	LDAPMessage *res,
	struct berval *newpasswd )
{
	int rc;
	char *retoid = NULL;
	struct berval *retdata;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( res != NULL );
	assert( newpasswd != NULL );

	newpasswd->bv_val = NULL;
	newpasswd->bv_len = 0;

	rc = ldap_parse_extended_result( ld, res, &retoid, &retdata, 0 );

	if( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if( retdata != NULL ) {
		ber_tag_t tag;
		BerElement *ber = ber_init( retdata );

		if( ber == NULL ) {
			ld->ld_errno = LDAP_NO_MEMORY;
			return ld->ld_errno;
		}

		/* we should check the tag */
		tag = ber_scanf( ber, "{o}", newpasswd );
		ber_free( ber, 1 );

		if( tag == LBER_ERROR ) {
			rc = ld->ld_errno = LDAP_DECODING_ERROR;
		}
	}

	ber_memfree( retoid );
	return rc;
}

int
ldap_passwd( LDAP *ld,
	struct berval	*user,
	struct berval	*oldpw,
	struct berval	*newpw,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	int				*msgidp )
{
	int rc;
	struct berval bv = {0, NULL};
	BerElement *ber = NULL;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msgidp != NULL );

	if( user != NULL || oldpw != NULL || newpw != NULL ) {
		/* build change password control */
		ber = ber_alloc_t( LBER_USE_DER );

		if( ber == NULL ) {
			ld->ld_errno = LDAP_NO_MEMORY;
			return ld->ld_errno;
		}

		ber_printf( ber, "{" /*}*/ );

		if( user != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_ID, user );
		}

		if( oldpw != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_OLD, oldpw );
		}

		if( newpw != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_NEW, newpw );
		}

		ber_printf( ber, /*{*/ "N}" );

		rc = ber_flatten2( ber, &bv, 0 );

		if( rc < 0 ) {
			ld->ld_errno = LDAP_ENCODING_ERROR;
			return ld->ld_errno;
		}

	}
	
	rc = ldap_extended_operation( ld, LDAP_EXOP_MODIFY_PASSWD,
		bv.bv_val ? &bv : NULL, sctrls, cctrls, msgidp );

	ber_free( ber, 1 );

	return rc;
}

int
ldap_passwd_s(
	LDAP *ld,
	struct berval	*user,
	struct berval	*oldpw,
	struct berval	*newpw,
	struct berval *newpasswd,
	LDAPControl **sctrls,
	LDAPControl **cctrls )
{
	int		rc;
	int		msgid;
	LDAPMessage	*res;

	rc = ldap_passwd( ld, user, oldpw, newpw, sctrls, cctrls, &msgid );
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if ( ldap_result( ld, msgid, 1, (struct timeval *) NULL, &res ) == -1 ) {
		return ld->ld_errno;
	}

	rc = ldap_parse_passwd( ld, res, newpasswd );
	if( rc != LDAP_SUCCESS ) {
		ldap_msgfree( res );
		return rc;
	}

	return( ldap_result2error( ld, res, 1 ) );
}
