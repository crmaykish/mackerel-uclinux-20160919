/* bind.c - decode an ldap bind operation and pass it to a backend db */
/* $OpenLDAP: pkg/ldap/servers/slapd/bind.c,v 1.109.2.15 2003/05/18 19:58:56 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

/*
 * Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "ldap_pvt.h"
#include "slap.h"
#ifdef LDAP_SLAPI
#include "slapi.h"
#endif


int
do_bind(
    Connection	*conn,
    Operation	*op
)
{
	BerElement *ber = op->o_ber;
	ber_int_t version;
	ber_tag_t method;
	struct berval mech = { 0, NULL };
	struct berval dn = { 0, NULL };
	struct berval pdn = { 0, NULL };
	struct berval ndn = { 0, NULL };
	struct berval edn = { 0, NULL };
	ber_tag_t tag;
	int	rc = LDAP_SUCCESS;
	const char *text;
	struct berval cred = { 0, NULL };
	Backend *be = NULL;

#ifdef LDAP_SLAPI
	Slapi_PBlock *pb = op->o_pb;
#endif

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ENTRY, "do_bind: conn %d\n", conn->c_connid, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "do_bind\n", 0, 0, 0 );
#endif

	/*
	 * Force to connection to "anonymous" until bind succeeds.
	 */
	ldap_pvt_thread_mutex_lock( &conn->c_mutex );
	if ( conn->c_sasl_bind_in_progress ) be = conn->c_authz_backend;

	/* log authorization identity demotion */
	if ( conn->c_dn.bv_len ) {
		Statslog( LDAP_DEBUG_STATS,
			"conn=%lu op=%lu BIND anonymous mech=implicit ssf=0\n",
			op->o_connid, op->o_opid, 0, 0, 0 );
	}

	connection2anonymous( conn );
	if ( conn->c_sasl_bind_in_progress ) conn->c_authz_backend = be;
	ldap_pvt_thread_mutex_unlock( &conn->c_mutex );

	if ( op->o_dn.bv_val != NULL ) {
		free( op->o_dn.bv_val );
		op->o_dn.bv_val = ch_strdup( "" );
		op->o_dn.bv_len = 0;
	}

	if ( op->o_ndn.bv_val != NULL ) {
		free( op->o_ndn.bv_val );
		op->o_ndn.bv_val = ch_strdup( "" );
		op->o_ndn.bv_len = 0;
	}

	/*
	 * Parse the bind request.  It looks like this:
	 *
	 *	BindRequest ::= SEQUENCE {
	 *		version		INTEGER,		 -- version
	 *		name		DistinguishedName,	 -- dn
	 *		authentication	CHOICE {
	 *			simple		[0] OCTET STRING -- passwd
	 *			krbv42ldap	[1] OCTET STRING
	 *			krbv42dsa	[2] OCTET STRING
	 *			SASL		[3] SaslCredentials
	 *		}
	 *	}
	 *
	 *	SaslCredentials ::= SEQUENCE {
	 *		mechanism	    LDAPString,
	 *		credentials	    OCTET STRING OPTIONAL
	 *	}
	 */

	tag = ber_scanf( ber, "{imt" /*}*/, &version, &dn, &method );

	if ( tag == LBER_ERROR ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"do_bind: conn %d  ber_scanf failed\n", conn->c_connid, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "bind: ber_scanf failed\n", 0, 0, 0 );
#endif
		send_ldap_disconnect( conn, op,
			LDAP_PROTOCOL_ERROR, "decoding error" );
		rc = -1;
		goto cleanup;
	}

	op->o_protocol = version;

	if( method != LDAP_AUTH_SASL ) {
		tag = ber_scanf( ber, /*{*/ "m}", &cred );

	} else {
		tag = ber_scanf( ber, "{o" /*}*/, &mech );

		if ( tag != LBER_ERROR ) {
			ber_len_t len;
			tag = ber_peek_tag( ber, &len );

			if ( tag == LDAP_TAG_LDAPCRED ) { 
				tag = ber_scanf( ber, "m", &cred );
			} else {
				tag = LDAP_TAG_LDAPCRED;
				cred.bv_val = NULL;
				cred.bv_len = 0;
			}

			if ( tag != LBER_ERROR ) {
				tag = ber_scanf( ber, /*{{*/ "}}" );
			}
		}
	}

	if ( tag == LBER_ERROR ) {
		send_ldap_disconnect( conn, op,
			LDAP_PROTOCOL_ERROR,
		"decoding error" );
		rc = SLAPD_DISCONNECT;
		goto cleanup;
	}

	if( (rc = get_ctrls( conn, op, 1 )) != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			"do_bind: conn %d  get_ctrls failed\n", conn->c_connid, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "do_bind: get_ctrls failed\n", 0, 0, 0 );
#endif
		goto cleanup;
	} 

	rc = dnPrettyNormal( NULL, &dn, &pdn, &ndn );
	if ( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			"do_bind: conn %d  invalid dn (%s)\n", 
			conn->c_connid, dn.bv_val, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "bind: invalid dn (%s)\n",
			dn.bv_val, 0, 0 );
#endif
		send_ldap_result( conn, op, rc = LDAP_INVALID_DN_SYNTAX, NULL,
		    "invalid DN", NULL, NULL );
		goto cleanup;
	}

	if( method == LDAP_AUTH_SASL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION,	 DETAIL1, 
			"do_sasl_bind: conn %d  dn (%s) mech %s\n", 
			conn->c_connid, pdn.bv_val, mech.bv_val );
#else
		Debug( LDAP_DEBUG_TRACE, "do_sasl_bind: dn (%s) mech %s\n",
			pdn.bv_val, mech.bv_val, NULL );
#endif

	} else {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, DETAIL1, 
			"do_bind: version=%ld dn=\"%s\" method=%ld\n",
			(unsigned long) version, pdn.bv_val, (unsigned long)method );
#else
		Debug( LDAP_DEBUG_TRACE,
			"do_bind: version=%ld dn=\"%s\" method=%ld\n",
			(unsigned long) version,
			pdn.bv_val, (unsigned long) method );
#endif
	}

	Statslog( LDAP_DEBUG_STATS, "conn=%lu op=%lu BIND dn=\"%s\" method=%ld\n",
	    op->o_connid, op->o_opid, pdn.bv_val, (unsigned long) method, 0 );

	if ( version < LDAP_VERSION_MIN || version > LDAP_VERSION_MAX ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			"do_bind: conn %d  unknown version = %ld\n",
			conn->c_connid, (unsigned long)version, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "do_bind: unknown version=%ld\n",
			(unsigned long) version, 0, 0 );
#endif
		send_ldap_result( conn, op, rc = LDAP_PROTOCOL_ERROR,
			NULL, "requested protocol version not supported", NULL, NULL );
		goto cleanup;

	} else if (!( global_allows & SLAP_ALLOW_BIND_V2 ) &&
		version < LDAP_VERSION3 )
	{
		send_ldap_result( conn, op, rc = LDAP_PROTOCOL_ERROR,
			NULL, "requested protocol version not allowed", NULL, NULL );
		goto cleanup;
	}

	/* we set connection version regardless of whether bind succeeds
	 * or not.
	 */
	ldap_pvt_thread_mutex_lock( &conn->c_mutex );
	conn->c_protocol = version;
	ldap_pvt_thread_mutex_unlock( &conn->c_mutex );

	/* check for inappropriate controls */
	if( get_manageDSAit( op ) == SLAP_CRITICAL_CONTROL ) {
		send_ldap_result( conn, op,
			rc = LDAP_UNAVAILABLE_CRITICAL_EXTENSION,
			NULL, "manageDSAit control inappropriate",
			NULL, NULL );
		goto cleanup;
	}

	/* Set the bindop for the benefit of in-directory SASL lookups */
	conn->c_sasl_bindop = op;

	if ( method == LDAP_AUTH_SASL ) {
		slap_ssf_t ssf = 0;

		if ( version < LDAP_VERSION3 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, INFO, 
				"do_bind: conn %d  sasl with LDAPv%ld\n",
				conn->c_connid, (unsigned long)version , 0 );
#else
			Debug( LDAP_DEBUG_ANY, "do_bind: sasl with LDAPv%ld\n",
				(unsigned long) version, 0, 0 );
#endif
			send_ldap_disconnect( conn, op,
				LDAP_PROTOCOL_ERROR, "SASL bind requires LDAPv3" );
			rc = SLAPD_DISCONNECT;
			goto cleanup;
		}

		if( mech.bv_len == 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, INFO, 
				   "do_bind: conn %d  no SASL mechanism provided\n",
				   conn->c_connid, 0, 0 );
#else
			Debug( LDAP_DEBUG_ANY,
				"do_bind: no sasl mechanism provided\n",
				0, 0, 0 );
#endif
			send_ldap_result( conn, op, rc = LDAP_AUTH_METHOD_NOT_SUPPORTED,
				NULL, "no SASL mechanism provided", NULL, NULL );
			goto cleanup;
		}

		/* check restrictions */
		rc = backend_check_restrictions( NULL, conn, op, &mech, &text );
		if( rc != LDAP_SUCCESS ) {
			send_ldap_result( conn, op, rc,
				NULL, text, NULL, NULL );
			goto cleanup;
		}

		ldap_pvt_thread_mutex_lock( &conn->c_mutex );
		if ( conn->c_sasl_bind_in_progress ) {
			if( !bvmatch( &conn->c_sasl_bind_mech, &mech ) ) {
				/* mechanism changed between bind steps */
				slap_sasl_reset(conn);
			}
		} else {
			conn->c_sasl_bind_mech = mech;
			mech.bv_val = NULL;
			mech.bv_len = 0;
		}
		ldap_pvt_thread_mutex_unlock( &conn->c_mutex );

		rc = slap_sasl_bind( conn, op,
			&pdn, &ndn,
			&cred, &edn, &ssf );

		ldap_pvt_thread_mutex_lock( &conn->c_mutex );
		if( rc == LDAP_SUCCESS ) {
			conn->c_dn = edn;
			if( edn.bv_len != 0 ) {
				/* edn is always normalized already */
				ber_dupbv( &conn->c_ndn, &conn->c_dn );
			}
			conn->c_authmech = conn->c_sasl_bind_mech;
			conn->c_sasl_bind_mech.bv_val = NULL;
			conn->c_sasl_bind_mech.bv_len = 0;
			conn->c_sasl_bind_in_progress = 0;

			conn->c_sasl_ssf = ssf;
			if( ssf > conn->c_ssf ) {
				conn->c_ssf = ssf;
			}

			if( conn->c_dn.bv_len != 0 ) {
				ber_len_t max = sockbuf_max_incoming_auth;
				ber_sockbuf_ctrl( conn->c_sb,
					LBER_SB_OPT_SET_MAX_INCOMING, &max );
			}

			/* log authorization identity */
			Statslog( LDAP_DEBUG_STATS,
				"conn=%lu op=%lu BIND dn=\"%s\" mech=%s ssf=%d\n",
				op->o_connid, op->o_opid,
				conn->c_dn.bv_val ? conn->c_dn.bv_val : "<empty>",
				conn->c_authmech.bv_val, ssf );

#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1, 
				"do_bind: SASL/%s bind: dn=\"%s\" ssf=%d\n",
				conn->c_authmech.bv_val,
				conn->c_dn.bv_val ? conn->c_dn.bv_val : "<empty>",
				ssf );
#else
			Debug( LDAP_DEBUG_TRACE,
				"do_bind: SASL/%s bind: dn=\"%s\" ssf=%d\n",
				conn->c_authmech.bv_val,
				conn->c_dn.bv_val ? conn->c_dn.bv_val : "<empty>",
				ssf );
#endif

		} else if ( rc == LDAP_SASL_BIND_IN_PROGRESS ) {
			conn->c_sasl_bind_in_progress = 1;

		} else {
			if ( conn->c_sasl_bind_mech.bv_val ) {
				free( conn->c_sasl_bind_mech.bv_val );
				conn->c_sasl_bind_mech.bv_val = NULL;
				conn->c_sasl_bind_mech.bv_len = 0;
			}
			conn->c_sasl_bind_in_progress = 0;
		}
		ldap_pvt_thread_mutex_unlock( &conn->c_mutex );

		goto cleanup;

	} else {
		/* Not SASL, cancel any in-progress bind */
		ldap_pvt_thread_mutex_lock( &conn->c_mutex );

		if ( conn->c_sasl_bind_mech.bv_val != NULL ) {
			free(conn->c_sasl_bind_mech.bv_val);
			conn->c_sasl_bind_mech.bv_val = NULL;
			conn->c_sasl_bind_mech.bv_len = 0;
		}
		conn->c_sasl_bind_in_progress = 0;

		slap_sasl_reset( conn );
		ldap_pvt_thread_mutex_unlock( &conn->c_mutex );
	}

	if ( method == LDAP_AUTH_SIMPLE ) {
		/* accept "anonymous" binds */
		if ( cred.bv_len == 0 || ndn.bv_len == 0 ) {
			rc = LDAP_SUCCESS;
			text = NULL;

			if( cred.bv_len &&
				!( global_allows & SLAP_ALLOW_BIND_ANON_CRED ))
			{
				/* cred is not empty, disallow */
				rc = LDAP_INVALID_CREDENTIALS;

			} else if ( ndn.bv_len &&
				!( global_allows & SLAP_ALLOW_BIND_ANON_DN ))
			{
				/* DN is not empty, disallow */
				rc = LDAP_UNWILLING_TO_PERFORM;
				text = "unauthenticated bind (DN with no password) disallowed";

			} else if ( global_disallows & SLAP_DISALLOW_BIND_ANON ) {
				/* disallow */
				rc = LDAP_INAPPROPRIATE_AUTH;
				text = "anonymous bind disallowed";

			} else {
				rc = backend_check_restrictions( NULL, conn, op,
					&mech, &text );
			}

			/*
			 * we already forced connection to "anonymous",
			 * just need to send success
			 */
			send_ldap_result( conn, op, rc,
				NULL, text, NULL, NULL );
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1, 
				   "do_bind: conn %d  v%d anonymous bind\n",
				   conn->c_connid, version , 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "do_bind: v%d anonymous bind\n",
				version, 0, 0 );
#endif
			goto cleanup;

		} else if ( global_disallows & SLAP_DISALLOW_BIND_SIMPLE ) {
			/* disallow simple authentication */
			rc = LDAP_UNWILLING_TO_PERFORM;
			text = "unwilling to perform simple authentication";

			send_ldap_result( conn, op, rc,
				NULL, text, NULL, NULL );
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, INFO, 
				   "do_bind: conn %d  v%d simple bind(%s) disallowed\n",
				   conn->c_connid, version, ndn.bv_val );
#else
			Debug( LDAP_DEBUG_TRACE,
				"do_bind: v%d simple bind(%s) disallowed\n",
				version, ndn.bv_val, 0 );
#endif
			goto cleanup;

		} else if (( global_disallows & SLAP_DISALLOW_BIND_SIMPLE_UNPROTECTED )
			&& ( op->o_ssf <= 1 ))
		{
			rc = LDAP_CONFIDENTIALITY_REQUIRED;
			text = "unwilling to perform simple authentication "
				"without confidentilty protection";

			send_ldap_result( conn, op, rc,
				NULL, text, NULL, NULL );

#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, INFO, "do_bind: conn %d  "
				"v%d unprotected simple bind(%s) disallowed\n",
				conn->c_connid, version, ndn.bv_val );
#else
			Debug( LDAP_DEBUG_TRACE,
				"do_bind: v%d unprotected simple bind(%s) disallowed\n",
				version, ndn.bv_val, 0 );
#endif
			goto cleanup;
		}

#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
	} else if ( method == LDAP_AUTH_KRBV41 || method == LDAP_AUTH_KRBV42 ) {
		if ( global_disallows & SLAP_DISALLOW_BIND_KRBV4 ) {
			/* disallow simple authentication */
			rc = LDAP_UNWILLING_TO_PERFORM;
			text = "unwilling to perform Kerberos V4 bind";

			send_ldap_result( conn, op, rc,
				NULL, text, NULL, NULL );
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1, 
				   "do_bind: conn %d  v%d Kerberos V4 bind\n",
				   conn->c_connid, version , 0 );
#else
			Debug( LDAP_DEBUG_TRACE, "do_bind: v%d Kerberos V4 bind\n",
				version, 0, 0 );
#endif
			goto cleanup;
		}
#endif

	} else {
		rc = LDAP_AUTH_METHOD_NOT_SUPPORTED;
		text = "unknown authentication method";

		send_ldap_result( conn, op, rc,
			NULL, text, NULL, NULL );
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			   "do_bind: conn %ld  v%d unknown authentication method (%ld)\n",
			   conn->c_connid, version, method );
#else
		Debug( LDAP_DEBUG_TRACE,
			"do_bind: v%d unknown authentication method (%ld)\n",
			version, method, 0 );
#endif
		goto cleanup;
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */

	if ( (be = select_backend( &ndn, 0, 0 )) == NULL ) {
		if ( default_referral ) {
			BerVarray ref = referral_rewrite( default_referral,
				NULL, &pdn, LDAP_SCOPE_DEFAULT );

			send_ldap_result( conn, op, rc = LDAP_REFERRAL,
				NULL, NULL, ref ? ref : default_referral, NULL );

			ber_bvarray_free( ref );

		} else {
			/* noSuchObject is not allowed to be returned by bind */
			send_ldap_result( conn, op, rc = LDAP_INVALID_CREDENTIALS,
				NULL, NULL, NULL, NULL );
		}

		goto cleanup;
	}

	/* check restrictions */
	rc = backend_check_restrictions( be, conn, op, NULL, &text );
	if( rc != LDAP_SUCCESS ) {
		send_ldap_result( conn, op, rc,
			NULL, text, NULL, NULL );
		goto cleanup;
	}

#if defined( LDAP_SLAPI )
	slapi_x_backend_set_pb( pb, be );
	slapi_x_connection_set_pb( pb, conn );
	slapi_x_operation_set_pb( pb, op );
	slapi_pblock_set( pb, SLAPI_BIND_TARGET, (void *)dn.bv_val );
	slapi_pblock_set( pb, SLAPI_BIND_METHOD, (void *)method );
	slapi_pblock_set( pb, SLAPI_BIND_CREDENTIALS, (void *)&cred );
	slapi_pblock_set( pb, SLAPI_MANAGEDSAIT, (void *)(0) );

	rc = doPluginFNs( be, SLAPI_PLUGIN_PRE_BIND_FN, pb );
	if ( rc != SLAPI_BIND_SUCCESS ) {
		/*
		 * Binding is a special case for SLAPI plugins. It is
		 * possible for a bind plugin to be successful *and*
		 * abort further processing; this means it has handled
		 * a bind request authoritatively. If we have reached
		 * here, a result has been sent to the client (XXX
		 * need to check with Sun whether SLAPI_BIND_ANONYMOUS
		 * means a result has been sent).
		 */
		int ldapRc;

		if ( slapi_pblock_get( pb, SLAPI_RESULT_CODE, (void *)&ldapRc ) != 0 )
			ldapRc = LDAP_OTHER;

		edn.bv_val = NULL;
		edn.bv_len = 0;
		if ( rc != SLAPI_BIND_FAIL && ldapRc == LDAP_SUCCESS ) {
			/* Set the new connection DN. */
			if ( rc != SLAPI_BIND_ANONYMOUS ) {
				slapi_pblock_get( pb, SLAPI_CONN_DN, (void *)&edn.bv_val );
			}
			rc = dnPrettyNormal( NULL, &edn, &pdn, &ndn );
			ldap_pvt_thread_mutex_lock( &conn->c_mutex );
			conn->c_dn = pdn;
			conn->c_ndn = ndn;
			pdn.bv_val = NULL;
			pdn.bv_len = 0;
			ndn.bv_val = NULL;
			ndn.bv_len = 0;
			if ( conn->c_dn.bv_len != 0 ) {
				ber_len_t max = sockbuf_max_incoming_auth;
				ber_sockbuf_ctrl( conn->c_sb, LBER_SB_OPT_SET_MAX_INCOMING, &max );
			}
			/* log authorization identity */
			Statslog( LDAP_DEBUG_STATS,
				"conn=%lu op=%lu BIND dn=\"%s\" mech=simple (SLAPI) ssf=0\n",
				op->o_connid, op->o_opid,
				conn->c_dn.bv_val, 0, 0 );
			ldap_pvt_thread_mutex_unlock( &conn->c_mutex );
		}
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, "do_bind: Bind preoperation plugin returned %d\n",
				rc, 0, 0);
#else
		Debug(LDAP_DEBUG_TRACE, "do_bind: Bind preoperation plugin returned %d.\n",
				rc, 0, 0);
#endif
		rc = ldapRc;
		goto cleanup;
	}
#endif /* defined( LDAP_SLAPI ) */

	if ( be->be_bind ) {
		int ret;

		ret = (*be->be_bind)( be, conn, op,
			&pdn, &ndn, method, &cred, &edn );

		if ( ret == 0 ) {
			ldap_pvt_thread_mutex_lock( &conn->c_mutex );

			if( conn->c_authz_backend == NULL ) {
				conn->c_authz_backend = be;
			}

			if(edn.bv_len) {
				conn->c_dn = edn;
			} else {
				conn->c_dn = pdn;
				pdn.bv_val = NULL;
				pdn.bv_len = 0;
			}

			conn->c_ndn = ndn;
			ndn.bv_val = NULL;
			ndn.bv_len = 0;

			if( conn->c_dn.bv_len != 0 ) {
				ber_len_t max = sockbuf_max_incoming_auth;
				ber_sockbuf_ctrl( conn->c_sb,
					LBER_SB_OPT_SET_MAX_INCOMING, &max );
			}

			/* log authorization identity */
			Statslog( LDAP_DEBUG_STATS,
				"conn=%lu op=%lu BIND dn=\"%s\" mech=simple ssf=0\n",
				op->o_connid, op->o_opid,
				conn->c_dn.bv_val, conn->c_authmech.bv_val, 0 );

#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, DETAIL1, 
				"do_bind: v%d bind: \"%s\" to \"%s\" \n",
				version, conn->c_dn.bv_val, conn->c_dn.bv_val );
#else
			Debug( LDAP_DEBUG_TRACE,
				"do_bind: v%d bind: \"%s\" to \"%s\"\n",
				version, dn.bv_val, conn->c_dn.bv_val );
#endif

			ldap_pvt_thread_mutex_unlock( &conn->c_mutex );

			/* send this here to avoid a race condition */
			send_ldap_result( conn, op, LDAP_SUCCESS,
				NULL, NULL, NULL, NULL );

		} else if (edn.bv_val != NULL) {
			free( edn.bv_val );
		}

	} else {
		send_ldap_result( conn, op, rc = LDAP_UNWILLING_TO_PERFORM,
			NULL, "operation not supported within namingContext",
			NULL, NULL );
	}

#if defined( LDAP_SLAPI )
	if ( doPluginFNs( be, SLAPI_PLUGIN_POST_BIND_FN, pb ) != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, "do_bind: Bind postoperation plugins failed\n",
				0, 0, 0);
#else
		Debug(LDAP_DEBUG_TRACE, "do_bind: Bind postoperation plugins failed.\n",
				0, 0, 0);
#endif
	}
#endif /* defined( LDAP_SLAPI ) */

cleanup:
	conn->c_sasl_bindop = NULL;

	if( pdn.bv_val != NULL ) {
		free( pdn.bv_val );
	}
	if( ndn.bv_val != NULL ) {
		free( ndn.bv_val );
	}
	if ( mech.bv_val != NULL ) {
		free( mech.bv_val );
	}

	return rc;
}
