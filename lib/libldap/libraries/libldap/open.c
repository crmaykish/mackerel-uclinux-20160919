/* $OpenLDAP: pkg/ldap/libraries/libldap/open.c,v 1.85.2.9 2003/04/28 23:41:55 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*  Portions
 *  Copyright (c) 1995 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  open.c
 */

#include "portable.h"

#include <stdio.h>
#include <limits.h>

#include <ac/stdlib.h>

#include <ac/param.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include <ac/unistd.h>

#include "ldap-int.h"
#include "ldap_log.h"

int ldap_open_defconn( LDAP *ld )
{
	ld->ld_defconn = ldap_new_connection( ld,
		ld->ld_options.ldo_defludp, 1, 1, NULL );

	if( ld->ld_defconn == NULL ) {
		ld->ld_errno = LDAP_SERVER_DOWN;
		return -1;
	}

	++ld->ld_defconn->lconn_refcnt;	/* so it never gets closed/freed */
	return 0;
}

/*
 * ldap_open - initialize and connect to an ldap server.  A magic cookie to
 * be used for future communication is returned on success, NULL on failure.
 * "host" may be a space-separated list of hosts or IP addresses
 *
 * Example:
 *	LDAP	*ld;
 *	ld = ldap_open( hostname, port );
 */

LDAP *
ldap_open( LDAP_CONST char *host, int port )
{
	int rc;
	LDAP		*ld;

#ifdef NEW_LOGGING
	LDAP_LOG ( CONNECTION, ARGS, "ldap_open(%s, %d)\n", host, port, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_open(%s, %d)\n",
		host, port, 0 );
#endif

	ld = ldap_init( host, port );
	if ( ld == NULL ) {
		return( NULL );
	}

	rc = ldap_open_defconn( ld );

	if( rc < 0 ) {
		ldap_ld_free( ld, 0, NULL, NULL );
		ld = NULL;
	}

#ifdef NEW_LOGGING
	LDAP_LOG ( CONNECTION, RESULTS, "ldap_open: %s\n",
		ld == NULL ? "succeeded" : "failed", 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_open: %s\n",
		ld == NULL ? "succeeded" : "failed", 0, 0 );
#endif

	return ld;
}



int
ldap_create( LDAP **ldp )
{
	LDAP			*ld;
	struct ldapoptions	*gopts;

	*ldp = NULL;
	/* Get pointer to global option structure */
	if ( (gopts = LDAP_INT_GLOBAL_OPT()) == NULL) {
		return LDAP_NO_MEMORY;
	}

	/* Initialize the global options, if not already done. */
	if( gopts->ldo_valid != LDAP_INITIALIZED ) {
		ldap_int_initialize(gopts, NULL);
		if ( gopts->ldo_valid != LDAP_INITIALIZED )
			return LDAP_LOCAL_ERROR;
	}

#ifdef NEW_LOGGING
	LDAP_LOG ( CONNECTION, ENTRY, "ldap_create\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_create\n", 0, 0, 0 );
#endif

	if ( (ld = (LDAP *) LDAP_CALLOC( 1, sizeof(LDAP) )) == NULL ) {
		return( LDAP_NO_MEMORY );
	}
   
	/* copy the global options */
	AC_MEMCPY(&ld->ld_options, gopts, sizeof(ld->ld_options));

	ld->ld_valid = LDAP_VALID_SESSION;

	/* but not pointers to malloc'ed items */
	ld->ld_options.ldo_sctrls = NULL;
	ld->ld_options.ldo_cctrls = NULL;

#ifdef HAVE_CYRUS_SASL
	ld->ld_options.ldo_def_sasl_mech = gopts->ldo_def_sasl_mech
		? LDAP_STRDUP( gopts->ldo_def_sasl_mech ) : NULL;
	ld->ld_options.ldo_def_sasl_realm = gopts->ldo_def_sasl_realm
		? LDAP_STRDUP( gopts->ldo_def_sasl_realm ) : NULL;
	ld->ld_options.ldo_def_sasl_authcid = gopts->ldo_def_sasl_authcid
		? LDAP_STRDUP( gopts->ldo_def_sasl_authcid ) : NULL;
	ld->ld_options.ldo_def_sasl_authzid = gopts->ldo_def_sasl_authzid
		? LDAP_STRDUP( gopts->ldo_def_sasl_authzid ) : NULL;
#endif

	ld->ld_options.ldo_defludp = ldap_url_duplist(gopts->ldo_defludp);

	if ( ld->ld_options.ldo_defludp == NULL ) {
		LDAP_FREE( (char*)ld );
		return LDAP_NO_MEMORY;
	}

	if (( ld->ld_selectinfo = ldap_new_select_info()) == NULL ) {
		ldap_free_urllist( ld->ld_options.ldo_defludp );
		LDAP_FREE( (char*) ld );
		return LDAP_NO_MEMORY;
	}

	ld->ld_lberoptions = LBER_USE_DER;

	ld->ld_sb = ber_sockbuf_alloc( );
	if ( ld->ld_sb == NULL ) {
		ldap_free_urllist( ld->ld_options.ldo_defludp );
		LDAP_FREE( (char*) ld );
		return LDAP_NO_MEMORY;
	}

	*ldp = ld;
	return LDAP_SUCCESS;
}

/*
 * ldap_init - initialize the LDAP library.  A magic cookie to be used for
 * future communication is returned on success, NULL on failure.
 * "host" may be a space-separated list of hosts or IP addresses
 *
 * Example:
 *	LDAP	*ld;
 *	ld = ldap_init( host, port );
 */
LDAP *
ldap_init( LDAP_CONST char *defhost, int defport )
{
	LDAP *ld;
	int rc;

	rc = ldap_create(&ld);
	if ( rc != LDAP_SUCCESS )
		return NULL;

	if (defport != 0)
		ld->ld_options.ldo_defport = defport;

	if (defhost != NULL) {
		rc = ldap_set_option(ld, LDAP_OPT_HOST_NAME, defhost);
		if ( rc != LDAP_SUCCESS ) {
			ldap_ld_free(ld, 1, NULL, NULL);
			return NULL;
		}
	}

	return( ld );
}


int
ldap_initialize( LDAP **ldp, LDAP_CONST char *url )
{
	int rc;
	LDAP *ld;

	*ldp = NULL;
	rc = ldap_create(&ld);
	if ( rc != LDAP_SUCCESS )
		return rc;

	if (url != NULL) {
		rc = ldap_set_option(ld, LDAP_OPT_URI, url);
		if ( rc != LDAP_SUCCESS ) {
			ldap_ld_free(ld, 1, NULL, NULL);
			return rc;
		}
#ifdef LDAP_CONNECTIONLESS
		if (ldap_is_ldapc_url(url))
			LDAP_IS_UDP(ld) = 1;
#endif
	}

	*ldp = ld;
	return LDAP_SUCCESS;
}

int
ldap_int_open_connection(
	LDAP *ld,
	LDAPConn *conn,
	LDAPURLDesc *srv,
	int async )
{
	int rc = -1;
#ifdef HAVE_CYRUS_SASL
	char *sasl_host = NULL;
#endif
	char *host;
	int port, proto;

#ifdef NEW_LOGGING
	LDAP_LOG ( CONNECTION, ENTRY, "ldap_int_open_connection\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_int_open_connection\n", 0, 0, 0 );
#endif

	switch ( proto = ldap_pvt_url_scheme2proto( srv->lud_scheme ) ) {
		case LDAP_PROTO_TCP:
			port = srv->lud_port;

			if ( srv->lud_host == NULL || *srv->lud_host == 0 ) {
				host = NULL;
			} else {
				host = srv->lud_host;
			}

			if( !port ) {
				if( strcmp(srv->lud_scheme, "ldaps") == 0 ) {
					port = LDAPS_PORT;
				} else {
					port = LDAP_PORT;
				}
			}

			rc = ldap_connect_to_host( ld, conn->lconn_sb,
				proto, host, port, async );

			if ( rc == -1 ) return rc;

#ifdef LDAP_DEBUG
			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_debug,
				LBER_SBIOD_LEVEL_PROVIDER, (void *)"tcp_" );
#endif
			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_tcp,
				LBER_SBIOD_LEVEL_PROVIDER, NULL );

#ifdef HAVE_CYRUS_SASL
			sasl_host = ldap_host_connected_to( conn->lconn_sb );
#endif
			break;

#ifdef LDAP_CONNECTIONLESS
		case LDAP_PROTO_UDP:
			port = srv->lud_port;

			if ( srv->lud_host == NULL || *srv->lud_host == 0 ) {
				host = NULL;
			} else {
				host = srv->lud_host;
			}

			if( !port ) port = LDAP_PORT;

			LDAP_IS_UDP(ld) = 1;
			rc = ldap_connect_to_host( ld, conn->lconn_sb,
				proto, host, port, async );

			if ( rc == -1 ) return rc;
#ifdef LDAP_DEBUG
			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_debug,
				LBER_SBIOD_LEVEL_PROVIDER, (void *)"udp_" );
#endif
			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_udp,
				LBER_SBIOD_LEVEL_PROVIDER, NULL );

			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_readahead,
				LBER_SBIOD_LEVEL_PROVIDER, NULL );

			break;
#endif
		case LDAP_PROTO_IPC:
#ifdef LDAP_PF_LOCAL
			/* only IPC mechanism supported is PF_LOCAL (PF_UNIX) */
			rc = ldap_connect_to_path( ld, conn->lconn_sb,
				srv->lud_host, async );
			if ( rc == -1 ) return rc;
#ifdef LDAP_DEBUG
			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_debug,
				LBER_SBIOD_LEVEL_PROVIDER, (void *)"ipc_" );
#endif
			ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_fd,
				LBER_SBIOD_LEVEL_PROVIDER, NULL );

#ifdef HAVE_CYRUS_SASL
			sasl_host = ldap_host_connected_to( conn->lconn_sb );
#endif
			break;
#endif /* LDAP_PF_LOCAL */
		default:
			return -1;
			break;
	}

#ifdef LDAP_DEBUG
	ber_sockbuf_add_io( conn->lconn_sb, &ber_sockbuf_io_debug,
		INT_MAX, (void *)"ldap_" );
#endif

#ifdef LDAP_CONNECTIONLESS
	if( proto == LDAP_PROTO_UDP ) return 0;
#endif

#ifdef HAVE_CYRUS_SASL
	/* establish Cyrus SASL context prior to starting TLS so
		that SASL EXTERNAL might be used */
	if( sasl_host != NULL ) {
		ldap_int_sasl_open( ld, conn, sasl_host );
		LDAP_FREE( sasl_host );
	}
#ifdef LDAP_PF_LOCAL
	if( proto == LDAP_PROTO_IPC ) {
		char authid[sizeof("uidNumber=4294967295+gidNumber=4294967295,"
			"cn=peercred,cn=external,cn=auth")];
		sprintf( authid, "uidNumber=%d+gidNumber=%d,"
			"cn=peercred,cn=external,cn=auth",
			(int) geteuid(), (int) getegid() );
		ldap_int_sasl_external( ld, conn, authid, LDAP_PVT_SASL_LOCAL_SSF );
	}
#endif
#endif

#ifdef HAVE_TLS
	if (ld->ld_options.ldo_tls_mode == LDAP_OPT_X_TLS_HARD ||
		strcmp( srv->lud_scheme, "ldaps" ) == 0 )
	{
		++conn->lconn_refcnt;	/* avoid premature free */

		rc = ldap_int_tls_start( ld, conn, srv );

		--conn->lconn_refcnt;

		if (rc != LDAP_SUCCESS) {
			return -1;
		}
	}
#endif

#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
	if ( conn->lconn_krbinstance == NULL ) {
		char *c;
		conn->lconn_krbinstance = ldap_host_connected_to( conn->lconn_sb );

		if( conn->lconn_krbinstance != NULL && 
		    ( c = strchr( conn->lconn_krbinstance, '.' )) != NULL ) {
			*c = '\0';
		}
	}
#endif /* LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND */

	return( 0 );
}


int ldap_open_internal_connection( LDAP **ldp, ber_socket_t *fdp )
{
	int rc;
	LDAPConn *c;
	LDAPRequest *lr;

	rc = ldap_create( ldp );
	if( rc != LDAP_SUCCESS ) {
		*ldp = NULL;
		return( rc );
	}

	/* Make it appear that a search request, msgid 0, was sent */
	lr = (LDAPRequest *)LDAP_CALLOC( 1, sizeof( LDAPRequest ));
	if( lr == NULL ) {
		ldap_unbind( *ldp );
		*ldp = NULL;
		return( LDAP_NO_MEMORY );
	}
	memset(lr, 0, sizeof( LDAPRequest ));
	lr->lr_msgid = 0;
	lr->lr_status = LDAP_REQST_INPROGRESS;
	lr->lr_res_errno = LDAP_SUCCESS;
	(*ldp)->ld_requests = lr;

	/* Attach the passed socket as the *LDAP's connection */
	c = ldap_new_connection( *ldp, NULL, 1, 0, NULL);
	if( c == NULL ) {
		ldap_unbind( *ldp );
		*ldp = NULL;
		return( LDAP_NO_MEMORY );
	}
	ber_sockbuf_ctrl( c->lconn_sb, LBER_SB_OPT_SET_FD, fdp );
#ifdef LDAP_DEBUG
	ber_sockbuf_add_io( c->lconn_sb, &ber_sockbuf_io_debug,
		LBER_SBIOD_LEVEL_PROVIDER, (void *)"int_" );
#endif
	ber_sockbuf_add_io( c->lconn_sb, &ber_sockbuf_io_tcp,
	  LBER_SBIOD_LEVEL_PROVIDER, NULL );
	(*ldp)->ld_defconn = c;

	/* Add the connection to the *LDAP's select pool */
	ldap_mark_select_read( *ldp, c->lconn_sb );
	ldap_mark_select_write( *ldp, c->lconn_sb );

	/* Make this connection an LDAP V3 protocol connection */
	rc = LDAP_VERSION3;
	ldap_set_option( *ldp, LDAP_OPT_PROTOCOL_VERSION, &rc );

	return( LDAP_SUCCESS );
}
