/* $OpenLDAP: pkg/ldap/libraries/libldap/sasl.c,v 1.41.2.7 2003/05/22 22:22:41 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
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
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/errno.h>

#include "ldap-int.h"

/*
 * ldap_sasl_bind - bind to the ldap server (and X.500).
 * The dn (usually NULL), mechanism, and credentials are provided.
 * The message id of the request initiated is provided upon successful
 * (LDAP_SUCCESS) return.
 *
 * Example:
 *	ldap_sasl_bind( ld, NULL, "mechanism",
 *		cred, NULL, NULL, &msgid )
 */

int
ldap_sasl_bind(
	LDAP			*ld,
	LDAP_CONST char	*dn,
	LDAP_CONST char	*mechanism,
	struct berval	*cred,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	int				*msgidp )
{
	BerElement	*ber;
	int rc;

#ifdef NEW_LOGGING
	LDAP_LOG ( TRANSPORT, ENTRY, "ldap_sasl_bind\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_sasl_bind\n", 0, 0, 0 );
#endif

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msgidp != NULL );

	/* check client controls */
	rc = ldap_int_client_controls( ld, cctrls );
	if( rc != LDAP_SUCCESS ) return rc;

	if( mechanism == LDAP_SASL_SIMPLE ) {
		if( dn == NULL && cred != NULL && cred->bv_len ) {
			/* use default binddn */
			dn = ld->ld_defbinddn;
		}

	} else if( ld->ld_version < LDAP_VERSION3 ) {
		ld->ld_errno = LDAP_NOT_SUPPORTED;
		return ld->ld_errno;
	}

	if ( dn == NULL ) {
		dn = "";
	}

	/* create a message to send */
	if ( (ber = ldap_alloc_ber_with_options( ld )) == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	assert( LBER_VALID( ber ) );

	if( mechanism == LDAP_SASL_SIMPLE ) {
		/* simple bind */
		rc = ber_printf( ber, "{it{istON}" /*}*/,
			++ld->ld_msgid, LDAP_REQ_BIND,
			ld->ld_version, dn, LDAP_AUTH_SIMPLE,
			cred );
		
	} else if ( cred == NULL || cred->bv_val == NULL ) {
		/* SASL bind w/o creditials */
		rc = ber_printf( ber, "{it{ist{sN}N}" /*}*/,
			++ld->ld_msgid, LDAP_REQ_BIND,
			ld->ld_version, dn, LDAP_AUTH_SASL,
			mechanism );

	} else {
		/* SASL bind w/ creditials */
		rc = ber_printf( ber, "{it{ist{sON}N}" /*}*/,
			++ld->ld_msgid, LDAP_REQ_BIND,
			ld->ld_version, dn, LDAP_AUTH_SASL,
			mechanism, cred );
	}

	if( rc == -1 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( -1 );
	}

	/* Put Server Controls */
	if( ldap_int_put_controls( ld, sctrls, ber ) != LDAP_SUCCESS ) {
		ber_free( ber, 1 );
		return ld->ld_errno;
	}

	if ( ber_printf( ber, /*{*/ "N}" ) == -1 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return ld->ld_errno;
	}


	/* send the message */
	*msgidp = ldap_send_initial_request( ld, LDAP_REQ_BIND, dn, ber );

	if(*msgidp < 0)
		return ld->ld_errno;

	return LDAP_SUCCESS;
}


int
ldap_sasl_bind_s(
	LDAP			*ld,
	LDAP_CONST char	*dn,
	LDAP_CONST char	*mechanism,
	struct berval	*cred,
	LDAPControl		**sctrls,
	LDAPControl		**cctrls,
	struct berval	**servercredp )
{
	int	rc, msgid;
	LDAPMessage	*result;
	struct berval	*scredp = NULL;

#ifdef NEW_LOGGING
	LDAP_LOG ( TRANSPORT, ENTRY, "ldap_sasl_bind_s\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_sasl_bind_s\n", 0, 0, 0 );
#endif

	/* do a quick !LDAPv3 check... ldap_sasl_bind will do the rest. */
	if( servercredp != NULL ) {
		if (ld->ld_version < LDAP_VERSION3) {
			ld->ld_errno = LDAP_NOT_SUPPORTED;
			return ld->ld_errno;
		}
		*servercredp = NULL;
	}

	rc = ldap_sasl_bind( ld, dn, mechanism, cred, sctrls, cctrls, &msgid );

	if ( rc != LDAP_SUCCESS ) {
		return( rc );
	}

#ifdef LDAP_CONNECTIONLESS
	if (LDAP_IS_UDP(ld)) {
		return( rc );
	}
#endif

	if ( ldap_result( ld, msgid, 1, NULL, &result ) == -1 ) {
		return( ld->ld_errno );	/* ldap_result sets ld_errno */
	}

	/* parse the results */
	scredp = NULL;
	if( servercredp != NULL ) {
		rc = ldap_parse_sasl_bind_result( ld, result, &scredp, 0 );
	}

	if ( rc != LDAP_SUCCESS && rc != LDAP_SASL_BIND_IN_PROGRESS ) {
		ldap_msgfree( result );
		return( rc );
	}

	rc = ldap_result2error( ld, result, 1 );

	if ( rc == LDAP_SUCCESS || rc == LDAP_SASL_BIND_IN_PROGRESS ) {
		if( servercredp != NULL ) {
			*servercredp = scredp;
			scredp = NULL;
		}
	}

	if ( scredp != NULL ) {
		ber_bvfree(scredp);
	}

	return rc;
}


/*
* Parse BindResponse:
*
*   BindResponse ::= [APPLICATION 1] SEQUENCE {
*     COMPONENTS OF LDAPResult,
*     serverSaslCreds  [7] OCTET STRING OPTIONAL }
*
*   LDAPResult ::= SEQUENCE {
*     resultCode      ENUMERATED,
*     matchedDN       LDAPDN,
*     errorMessage    LDAPString,
*     referral        [3] Referral OPTIONAL }
*/

int
ldap_parse_sasl_bind_result(
	LDAP			*ld,
	LDAPMessage		*res,
	struct berval	**servercredp,
	int				freeit )
{
	ber_int_t errcode;
	struct berval* scred;

	ber_tag_t tag;
	BerElement	*ber;

#ifdef NEW_LOGGING
	LDAP_LOG ( TRANSPORT, ENTRY, "ldap_parse_sasl_bind_result\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_parse_sasl_bind_result\n", 0, 0, 0 );
#endif

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( res != NULL );

	if( servercredp != NULL ) {
		if( ld->ld_version < LDAP_VERSION2 ) {
			return LDAP_NOT_SUPPORTED;
		}
		*servercredp = NULL;
	}

	if( res->lm_msgtype != LDAP_RES_BIND ) {
		ld->ld_errno = LDAP_PARAM_ERROR;
		return ld->ld_errno;
	}

	scred = NULL;

	if ( ld->ld_error ) {
		LDAP_FREE( ld->ld_error );
		ld->ld_error = NULL;
	}
	if ( ld->ld_matched ) {
		LDAP_FREE( ld->ld_matched );
		ld->ld_matched = NULL;
	}

	/* parse results */

	ber = ber_dup( res->lm_ber );

	if( ber == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	if ( ld->ld_version < LDAP_VERSION2 ) {
		tag = ber_scanf( ber, "{ia}",
			&errcode, &ld->ld_error );

		if( tag == LBER_ERROR ) {
			ber_free( ber, 0 );
			ld->ld_errno = LDAP_DECODING_ERROR;
			return ld->ld_errno;
		}

	} else {
		ber_len_t len;

		tag = ber_scanf( ber, "{iaa" /*}*/,
			&errcode, &ld->ld_matched, &ld->ld_error );

		if( tag == LBER_ERROR ) {
			ber_free( ber, 0 );
			ld->ld_errno = LDAP_DECODING_ERROR;
			return ld->ld_errno;
		}

		tag = ber_peek_tag(ber, &len);

		if( tag == LDAP_TAG_REFERRAL ) {
			/* skip 'em */
			if( ber_scanf( ber, "x" ) == LBER_ERROR ) {
				ber_free( ber, 0 );
				ld->ld_errno = LDAP_DECODING_ERROR;
				return ld->ld_errno;
			}

			tag = ber_peek_tag(ber, &len);
		}

		if( tag == LDAP_TAG_SASL_RES_CREDS ) {
			if( ber_scanf( ber, "O", &scred ) == LBER_ERROR ) {
				ber_free( ber, 0 );
				ld->ld_errno = LDAP_DECODING_ERROR;
				return ld->ld_errno;
			}
		}
	}

	ber_free( ber, 0 );

	if ( servercredp != NULL ) {
		*servercredp = scred;

	} else if ( scred != NULL ) {
		ber_bvfree( scred );
	}

	ld->ld_errno = errcode;

	if ( freeit ) {
		ldap_msgfree( res );
	}

	return( ld->ld_errno );
}

int
ldap_pvt_sasl_getmechs ( LDAP *ld, char **pmechlist )
{
	/* we need to query the server for supported mechs anyway */
	LDAPMessage *res, *e;
	char *attrs[] = { "supportedSASLMechanisms", NULL };
	char **values, *mechlist;
	int rc;

#ifdef NEW_LOGGING
	LDAP_LOG ( TRANSPORT, ENTRY, "ldap_pvt_sasl_getmech\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_pvt_sasl_getmech\n", 0, 0, 0 );
#endif

	rc = ldap_search_s( ld, "", LDAP_SCOPE_BASE,
		NULL, attrs, 0, &res );

	if ( rc != LDAP_SUCCESS ) {
		return ld->ld_errno;
	}
		
	e = ldap_first_entry( ld, res );
	if ( e == NULL ) {
		ldap_msgfree( res );
		if ( ld->ld_errno == LDAP_SUCCESS ) {
			ld->ld_errno = LDAP_NO_SUCH_OBJECT;
		}
		return ld->ld_errno;
	}

	values = ldap_get_values( ld, e, "supportedSASLMechanisms" );
	if ( values == NULL ) {
		ldap_msgfree( res );
		ld->ld_errno = LDAP_NO_SUCH_ATTRIBUTE;
		return ld->ld_errno;
	}

	mechlist = ldap_charray2str( values, " " );
	if ( mechlist == NULL ) {
		LDAP_VFREE( values );
		ldap_msgfree( res );
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	} 

	LDAP_VFREE( values );
	ldap_msgfree( res );

	*pmechlist = mechlist;

	return LDAP_SUCCESS;
}

/*
 * ldap_sasl_interactive_bind_s - interactive SASL authentication
 *
 * This routine uses interactive callbacks.
 *
 * LDAP_SUCCESS is returned upon success, the ldap error code
 * otherwise.
 */
int
ldap_sasl_interactive_bind_s(
	LDAP *ld,
	LDAP_CONST char *dn, /* usually NULL */
	LDAP_CONST char *mechs,
	LDAPControl **serverControls,
	LDAPControl **clientControls,
	unsigned flags,
	LDAP_SASL_INTERACT_PROC *interact,
	void *defaults )
{
	int rc;

#if defined( LDAP_R_COMPILE ) && defined( HAVE_CYRUS_SASL )
	ldap_pvt_thread_mutex_lock( &ldap_int_sasl_mutex );
#endif
#ifdef LDAP_CONNECTIONLESS
	if( LDAP_IS_UDP(ld) ) {
		/* Just force it to simple bind, silly to make the user
		 * ask all the time. No, we don't ever actually bind, but I'll
		 * let the final bind handler take care of saving the cdn.
		 */
		rc = ldap_simple_bind(ld, dn, NULL);
		return rc < 0 ? rc : 0;
	} else
#endif
	if( mechs == NULL || *mechs == '\0' ) {
		char *smechs;

		rc = ldap_pvt_sasl_getmechs( ld, &smechs );

		if( rc != LDAP_SUCCESS ) {
			goto done;
		}

#ifdef NEW_LOGGING
		LDAP_LOG ( TRANSPORT, DETAIL1, 
			"ldap_interactive_sasl_bind_s: server supports: %s\n", 
			smechs, 0, 0 );
#else
		Debug( LDAP_DEBUG_TRACE,
			"ldap_interactive_sasl_bind_s: server supports: %s\n",
			smechs, 0, 0 );
#endif

		mechs = smechs;

	} else {
#ifdef NEW_LOGGING
		LDAP_LOG ( TRANSPORT, DETAIL1, 
			"ldap_interactive_sasl_bind_s: user selected: %s\n", mechs, 0, 0 );
#else
		Debug( LDAP_DEBUG_TRACE,
			"ldap_interactive_sasl_bind_s: user selected: %s\n",
			mechs, 0, 0 );
#endif
	}

	rc = ldap_int_sasl_bind( ld, dn, mechs,
		serverControls, clientControls,
		flags, interact, defaults );

done:
#if defined( LDAP_R_COMPILE ) && defined( HAVE_CYRUS_SASL )
	ldap_pvt_thread_mutex_unlock( &ldap_int_sasl_mutex );
#endif

	return rc;
}
