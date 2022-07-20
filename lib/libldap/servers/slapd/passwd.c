/* bind.c - ldbm backend bind and unbind routines */
/* $OpenLDAP: pkg/ldap/servers/slapd/passwd.c,v 1.35.2.8 2003/03/03 17:10:07 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/krb.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include "slap.h"

#include <lber_pvt.h>
#include <lutil.h>

int passwd_extop(
	Connection *conn, Operation *op,
	const char *reqoid,
	struct berval *reqdata,
	char **rspoid,
	struct berval **rspdata,
	LDAPControl ***rspctrls,
	const char **text,
	BerVarray *refs )
{
	Backend *be;
	int rc;

	assert( reqoid != NULL );
	assert( strcmp( LDAP_EXOP_MODIFY_PASSWD, reqoid ) == 0 );

	if( op->o_dn.bv_len == 0 ) {
		*text = "only authenticated users may change passwords";
		return LDAP_STRONG_AUTH_REQUIRED;
	}

	ldap_pvt_thread_mutex_lock( &conn->c_mutex );
	be = conn->c_authz_backend;
	ldap_pvt_thread_mutex_unlock( &conn->c_mutex );

	if( be && !be->be_extended ) {
		*text = "operation not supported for current user";
		return LDAP_UNWILLING_TO_PERFORM;
	}

	{
		struct berval passwd = BER_BVC( LDAP_EXOP_MODIFY_PASSWD );
		rc = backend_check_restrictions( be, conn, op, &passwd, text );
	}

	if( rc != LDAP_SUCCESS ) {
		return rc;
	}

	if( be == NULL ) {
#ifdef HAVE_CYRUS_SASL
		rc = slap_sasl_setpass( conn, op,
			reqoid, reqdata,
			rspoid, rspdata, rspctrls,
			text );
#else
		*text = "no authz backend";
		rc = LDAP_OTHER;
#endif

#ifndef SLAPD_MULTIMASTER
	/* This does not apply to multi-master case */
	} else if( be->be_update_ndn.bv_len ) {
		/* we SHOULD return a referral in this case */
		*refs = referral_rewrite( be->be_update_refs,
			NULL, NULL, LDAP_SCOPE_DEFAULT );
			rc = LDAP_REFERRAL;
#endif /* !SLAPD_MULTIMASTER */

	} else {
		rc = be->be_extended(
			be, conn, op,
			reqoid, reqdata,
			rspoid, rspdata, rspctrls,
			text, refs );
	}

	return rc;
}

int slap_passwd_parse( struct berval *reqdata,
	struct berval *id,
	struct berval *oldpass,
	struct berval *newpass,
	const char **text )
{
	int rc = LDAP_SUCCESS;
	ber_tag_t tag;
	ber_len_t len;
	char berbuf[LBER_ELEMENT_SIZEOF];
	BerElement *ber = (BerElement *)berbuf;

	if( reqdata == NULL ) {
		return LDAP_SUCCESS;
	}

	if( reqdata->bv_len == 0 ) {
		*text = "empty request data field";
		return LDAP_PROTOCOL_ERROR;
	}

	/* ber_init2 uses reqdata directly, doesn't allocate new buffers */
	ber_init2( ber, reqdata, 0 );

	tag = ber_scanf( ber, "{" /*}*/ );

	if( tag != LBER_ERROR ) {
		tag = ber_peek_tag( ber, &len );
	}

	if( tag == LDAP_TAG_EXOP_MODIFY_PASSWD_ID ) {
		if( id == NULL ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "slap_passwd_parse: ID not allowed.\n", 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: ID not allowed.\n",
				0, 0, 0 );
#endif

			*text = "user must change own password";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_scanf( ber, "m", id );

		if( tag == LBER_ERROR ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "slap_passwd_parse:  ID parse failed.\n", 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: ID parse failed.\n",
				0, 0, 0 );
#endif

			goto decoding_error;
		}

		tag = ber_peek_tag( ber, &len);
	}

	if( tag == LDAP_TAG_EXOP_MODIFY_PASSWD_OLD ) {
		if( oldpass == NULL ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "slap_passwd_parse: OLD not allowed.\n" , 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: OLD not allowed.\n",
				0, 0, 0 );
#endif

			*text = "use bind to verify old password";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_scanf( ber, "m", oldpass );

		if( tag == LBER_ERROR ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "slap_passwd_parse:  ID parse failed.\n" , 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: ID parse failed.\n",
				0, 0, 0 );
#endif

			goto decoding_error;
		}

		tag = ber_peek_tag( ber, &len );
	}

	if( tag == LDAP_TAG_EXOP_MODIFY_PASSWD_NEW ) {
		if( newpass == NULL ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "slap_passwd_parse:  NEW not allowed.\n", 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: NEW not allowed.\n",
				0, 0, 0 );
#endif

			*text = "user specified passwords disallowed";
			rc = LDAP_UNWILLING_TO_PERFORM;
			goto done;
		}

		tag = ber_scanf( ber, "m", newpass );

		if( tag == LBER_ERROR ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "slap_passwd_parse:  OLD parse failed.\n", 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "slap_passwd_parse: OLD parse failed.\n",
				0, 0, 0 );
#endif

			goto decoding_error;
		}

		tag = ber_peek_tag( ber, &len );
	}

	if( len != 0 ) {
decoding_error:
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"slap_passwd_parse: decoding error, len=%ld\n", (long)len, 0, 0 );
#else
		Debug( LDAP_DEBUG_TRACE,
			"slap_passwd_parse: decoding error, len=%ld\n",
			(long) len, 0, 0 );
#endif

		*text = "data decoding error";
		rc = LDAP_PROTOCOL_ERROR;
	}

done:
	return rc;
}

struct berval * slap_passwd_return(
	struct berval		*cred )
{
	int rc;
	struct berval *bv = NULL;
	char berbuf[LBER_ELEMENT_SIZEOF];
	/* opaque structure, size unknown but smaller than berbuf */
	BerElement *ber = (BerElement *)berbuf;

	assert( cred != NULL );

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ENTRY, 
		"slap_passwd_return: %ld\n",(long)cred->bv_len, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "slap_passwd_return: %ld\n",
		(long) cred->bv_len, 0, 0 );
#endif
	
	ber_init_w_nullc( ber, LBER_USE_DER );

	rc = ber_printf( ber, "{tON}",
		LDAP_TAG_EXOP_MODIFY_PASSWD_GEN, cred );

	if( rc >= 0 ) {
		(void) ber_flatten( ber, &bv );
	}

	ber_free_buf( ber );

	return bv;
}

int
slap_passwd_check(
	Connection *conn,
	Attribute *a,
	struct berval *cred )
{
	int result = 1;
	struct berval *bv;

#if defined( SLAPD_CRYPT ) || defined( SLAPD_SPASSWD )
	ldap_pvt_thread_mutex_lock( &passwd_mutex );
#ifdef SLAPD_SPASSWD
	lutil_passwd_sasl_conn = conn->c_sasl_context;
#endif
#endif

	for ( bv = a->a_vals; bv->bv_val != NULL; bv++ ) {
		if( !lutil_passwd( bv, cred, NULL ) ) {
			result = 0;
			break;
		}
	}

#if defined( SLAPD_CRYPT ) || defined( SLAPD_SPASSWD )
#ifdef SLAPD_SPASSWD
	lutil_passwd_sasl_conn = NULL;
#endif
	ldap_pvt_thread_mutex_unlock( &passwd_mutex );
#endif

	return result;
}

void
slap_passwd_generate( struct berval *pass )
{
	struct berval *tmp;
#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ENTRY, "slap_passwd_generate: begin\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "slap_passwd_generate\n", 0, 0, 0 );
#endif
	/*
	 * generate passwords of only 8 characters as some getpass(3)
	 * implementations truncate at 8 characters.
	 */
	tmp = lutil_passwd_generate( 8 );
	if (tmp) {
		*pass = *tmp;
		free(tmp);
	} else {
		pass->bv_val = NULL;
		pass->bv_len = 0;
	}
}

void
slap_passwd_hash(
	struct berval * cred,
	struct berval * new )
{
	struct berval *tmp;
#ifdef LUTIL_SHA1_BYTES
	char* hash = default_passwd_hash ?  default_passwd_hash : "{SSHA}";
#else
	char* hash = default_passwd_hash ?  default_passwd_hash : "{SMD5}";
#endif
	

#if defined( SLAPD_CRYPT ) || defined( SLAPD_SPASSWD )
	ldap_pvt_thread_mutex_lock( &passwd_mutex );
#endif

	tmp = lutil_passwd_hash( cred , hash );
	
#if defined( SLAPD_CRYPT ) || defined( SLAPD_SPASSWD )
	ldap_pvt_thread_mutex_unlock( &passwd_mutex );
#endif

	if( tmp == NULL ) {
		new->bv_len = 0;
		new->bv_val = NULL;
	}

	*new = *tmp;
	free( tmp );
	return;
}
