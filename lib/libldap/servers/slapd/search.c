/* $OpenLDAP: pkg/ldap/servers/slapd/search.c,v 1.86.2.9 2003/02/10 19:22:47 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* Portions
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
#include "lutil.h"
#include "slap.h"

#ifdef LDAP_SLAPI
#include "slapi.h"
static char **anlist2charray( AttributeName *an );
static Slapi_PBlock *initSearchPlugin( Backend *be, Connection *conn, Operation *op,
	struct berval *base, int scope, int deref, int sizelimit, int timelimit,
	Filter *filter, struct berval *fstr, char **attrs,
	int attrsonly, int managedsait );
static int doPreSearchPluginFNs( Backend *be, Slapi_PBlock *pb );
static int doSearchRewriteFNs( Backend *be, Slapi_PBlock *pb, Filter **filter, struct berval *fstr );
static void doPostSearchPluginFNs( Backend *be, Slapi_PBlock *pb );
#endif /* LDAPI_SLAPI */

int
do_search(
    Connection	*conn,	/* where to send results */
    Operation	*op	/* info about the op to which we're responding */
) {
	ber_int_t	scope, deref, attrsonly;
	ber_int_t	sizelimit, timelimit;
	struct berval base = { 0, NULL };
	struct berval pbase = { 0, NULL };
	struct berval nbase = { 0, NULL };
	struct berval	fstr = { 0, NULL };
	Filter		*filter = NULL;
	AttributeName	*an = NULL;
	ber_len_t	siz, off, i;
	Backend		*be;
	int			rc;
	const char	*text;
	int			manageDSAit;
#ifdef LDAP_SLAPI
	Slapi_PBlock	*pb = NULL;
	char		**attrs = NULL;
#endif

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ENTRY, "do_search: conn %d\n", conn->c_connid, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "do_search\n", 0, 0, 0 );
#endif

	/*
	 * Parse the search request.  It looks like this:
	 *
	 *	SearchRequest := [APPLICATION 3] SEQUENCE {
	 *		baseObject	DistinguishedName,
	 *		scope		ENUMERATED {
	 *			baseObject	(0),
	 *			singleLevel	(1),
	 *			wholeSubtree	(2)
	 *		},
	 *		derefAliases	ENUMERATED {
	 *			neverDerefaliases	(0),
	 *			derefInSearching	(1),
	 *			derefFindingBaseObj	(2),
	 *			alwaysDerefAliases	(3)
	 *		},
	 *		sizelimit	INTEGER (0 .. 65535),
	 *		timelimit	INTEGER (0 .. 65535),
	 *		attrsOnly	BOOLEAN,
	 *		filter		Filter,
	 *		attributes	SEQUENCE OF AttributeType
	 *	}
	 */

	/* baseObject, scope, derefAliases, sizelimit, timelimit, attrsOnly */
	if ( ber_scanf( op->o_ber, "{miiiib" /*}*/,
		&base, &scope, &deref, &sizelimit,
	    &timelimit, &attrsonly ) == LBER_ERROR )
	{
		send_ldap_disconnect( conn, op,
			LDAP_PROTOCOL_ERROR, "decoding error" );
		rc = SLAPD_DISCONNECT;
		goto return_results;
	}

	switch( scope ) {
	case LDAP_SCOPE_BASE:
	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBTREE:
		break;
	default:
		send_ldap_result( conn, op, rc = LDAP_PROTOCOL_ERROR,
			NULL, "invalid scope", NULL, NULL );
		goto return_results;
	}

	switch( deref ) {
	case LDAP_DEREF_NEVER:
	case LDAP_DEREF_FINDING:
	case LDAP_DEREF_SEARCHING:
	case LDAP_DEREF_ALWAYS:
		break;
	default:
		send_ldap_result( conn, op, rc = LDAP_PROTOCOL_ERROR,
			NULL, "invalid deref", NULL, NULL );
		goto return_results;
	}

	rc = dnPrettyNormal( NULL, &base, &pbase, &nbase );
	if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"do_search: conn %d  invalid dn (%s)\n",
			conn->c_connid, base.bv_val, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"do_search: invalid dn (%s)\n", base.bv_val, 0, 0 );
#endif
		send_ldap_result( conn, op, rc = LDAP_INVALID_DN_SYNTAX, NULL,
		    "invalid DN", NULL, NULL );
		goto return_results;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ARGS, "SRCH \"%s\" %d %d",
		base.bv_val, scope, deref );
	LDAP_LOG( OPERATION, ARGS, "    %d %d %d\n",
		sizelimit, timelimit, attrsonly);
#else
	Debug( LDAP_DEBUG_ARGS, "SRCH \"%s\" %d %d",
		base.bv_val, scope, deref );
	Debug( LDAP_DEBUG_ARGS, "    %d %d %d\n",
		sizelimit, timelimit, attrsonly);
#endif

	/* filter - returns a "normalized" version */
	rc = get_filter( conn, op->o_ber, &filter, &text );
	if( rc != LDAP_SUCCESS ) {
		if( rc == SLAPD_DISCONNECT ) {
			send_ldap_disconnect( conn, op,
				LDAP_PROTOCOL_ERROR, text );
		} else {
			send_ldap_result( conn, op, rc, 
					NULL, text, NULL, NULL );
		}
		goto return_results;
	}
	filter2bv( filter, &fstr );

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ARGS, 
		"do_search: conn %d	filter: %s\n", 
		conn->c_connid, fstr.bv_len ? fstr.bv_val : "empty", 0 );
#else
	Debug( LDAP_DEBUG_ARGS, "    filter: %s\n",
		fstr.bv_len ? fstr.bv_val : "empty", 0, 0 );
#endif

	/* attributes */
	siz = sizeof(AttributeName);
	off = 0;
	if ( ber_scanf( op->o_ber, "{M}}", &an, &siz, off ) == LBER_ERROR ) {
		send_ldap_disconnect( conn, op,
			LDAP_PROTOCOL_ERROR, "decoding attrs error" );
		rc = SLAPD_DISCONNECT;
		goto return_results;
	}
	for ( i=0; i<siz; i++ ) {
		an[i].an_desc = NULL;
		an[i].an_oc = NULL;
		slap_bv2ad(&an[i].an_name, &an[i].an_desc, &text);
	}

	if( (rc = get_ctrls( conn, op, 1 )) != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, 
			"do_search: conn %d  get_ctrls failed (%d)\n",
			conn->c_connid, rc, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "do_search: get_ctrls failed\n", 0, 0, 0 );
#endif

		goto return_results;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ARGS, 
		"do_search: conn %d	attrs:", conn->c_connid, 0, 0 );
#else
	Debug( LDAP_DEBUG_ARGS, "    attrs:", 0, 0, 0 );
#endif

	if ( siz != 0 ) {
		for ( i = 0; i<siz; i++ ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ARGS, 
				"do_search: %s", an[i].an_name.bv_val, 0, 0 );
#else
			Debug( LDAP_DEBUG_ARGS, " %s", an[i].an_name.bv_val, 0, 0 );
#endif
		}
	}

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, ARGS, "\n" , 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ARGS, "\n", 0, 0, 0 );
#endif

	if ( StatslogTest( LDAP_DEBUG_STATS ) ) {
		char abuf[BUFSIZ/2], *ptr = abuf;
		int len = 0, alen;

		Statslog( LDAP_DEBUG_STATS,
	    		"conn=%lu op=%lu SRCH base=\"%s\" scope=%d filter=\"%s\"\n",
	    		op->o_connid, op->o_opid, pbase.bv_val, scope, fstr.bv_val );

		for ( i = 0; i<siz; i++ ) {
			alen = an[i].an_name.bv_len;
			if (alen >= sizeof(abuf)) {
				alen = sizeof(abuf)-1;
			}
			if (len && (len + 1 + alen >= sizeof(abuf))) {
				Statslog( LDAP_DEBUG_STATS, "conn=%lu op=%lu SRCH attr=%s\n",
				    op->o_connid, op->o_opid, abuf, 0, 0 );
	    			len = 0;
				ptr = abuf;
			}
			if (len) {
				*ptr++ = ' ';
				len++;
			}
			ptr = lutil_strncopy(ptr, an[i].an_name.bv_val, alen);
			len += alen;
			*ptr = '\0';
		}
		if (len) {
			Statslog( LDAP_DEBUG_STATS, "conn=%lu op=%lu SRCH attr=%s\n",
	    			op->o_connid, op->o_opid, abuf, 0, 0 );
		}
	}

	manageDSAit = get_manageDSAit( op );

	if ( scope == LDAP_SCOPE_BASE ) {
		Entry *entry = NULL;

		if ( nbase.bv_len == 0 ) {
#ifdef LDAP_CONNECTIONLESS
			/* Ignore LDAPv2 CLDAP Root DSE queries */
			if (op->o_protocol == LDAP_VERSION2 && conn->c_is_udp) {
				goto return_results;
			}
#endif
			/* check restrictions */
			rc = backend_check_restrictions( NULL, conn, op, NULL, &text ) ;
			if( rc != LDAP_SUCCESS ) {
				send_ldap_result( conn, op, rc,
					NULL, text, NULL, NULL );
				goto return_results;
			}

#ifdef LDAP_SLAPI
			attrs = anlist2charray( an );
			pb = initSearchPlugin( NULL, conn, op, &nbase, scope,
				deref, sizelimit, timelimit, filter, &fstr,
				attrs, attrsonly, manageDSAit );
			rc = doPreSearchPluginFNs( NULL, pb );
			if ( rc == LDAP_SUCCESS ) {
				doSearchRewriteFNs( NULL, pb, &filter, &fstr );
#endif /* LDAP_SLAPI */
			rc = root_dse_info( conn, &entry, &text );
#ifdef LDAP_SLAPI
			}
#endif /* LDAP_SLAPI */

		} else if ( bvmatch( &nbase, &global_schemandn ) ) {
			/* check restrictions */
			rc = backend_check_restrictions( NULL, conn, op, NULL, &text ) ;
			if( rc != LDAP_SUCCESS ) {
				send_ldap_result( conn, op, rc,
					NULL, text, NULL, NULL );
				goto return_results;
			}

#ifdef LDAP_SLAPI
			attrs = anlist2charray( an );
			pb = initSearchPlugin( NULL, conn, op, &nbase, scope,
				deref, sizelimit, timelimit, filter, &fstr,
				attrs, attrsonly, manageDSAit );
			rc = doPreSearchPluginFNs( NULL, pb );
			if ( rc == LDAP_SUCCESS ) {
				doSearchRewriteFNs( NULL, pb, &filter, &fstr );
#endif /* LDAP_SLAPI */
			rc = schema_info( &entry, &text );
#ifdef LDAP_SLAPI
			}
#endif /* LDAP_SLAPI */
		}

		if( rc != LDAP_SUCCESS ) {
			send_ldap_result( conn, op, rc,
				NULL, text, NULL, NULL );
#ifdef LDAP_SLAPI
			doPostSearchPluginFNs( NULL, pb );
#endif /* LDAP_SLAPI */
			goto return_results;

		} else if ( entry != NULL ) {
			rc = test_filter( NULL, conn, op,
				entry, filter );

			if( rc == LDAP_COMPARE_TRUE ) {
				send_search_entry( NULL, conn, op,
					entry, an, attrsonly, NULL );
			}
			entry_free( entry );

			send_ldap_result( conn, op, LDAP_SUCCESS,
				NULL, NULL, NULL, NULL );
#ifdef LDAP_SLAPI
			doPostSearchPluginFNs( NULL, pb );
#endif /* LDAP_SLAPI */
			goto return_results;
		}
	}

	if( !nbase.bv_len && default_search_nbase.bv_len ) {
		ch_free( pbase.bv_val );
		ch_free( nbase.bv_val );

		ber_dupbv( &pbase, &default_search_base );
		ber_dupbv( &nbase, &default_search_nbase );
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	if ( (be = select_backend( &nbase, manageDSAit, 1 )) == NULL ) {
		BerVarray ref = referral_rewrite( default_referral,
			NULL, &pbase, scope );

		send_ldap_result( conn, op, rc = LDAP_REFERRAL,
			NULL, NULL, ref ? ref : default_referral, NULL );

		ber_bvarray_free( ref );
		goto return_results;
	}

	/* check restrictions */
	rc = backend_check_restrictions( be, conn, op, NULL, &text ) ;
	if( rc != LDAP_SUCCESS ) {
		send_ldap_result( conn, op, rc,
			NULL, text, NULL, NULL );
		goto return_results;
	}

	/* check for referrals */
	rc = backend_check_referrals( be, conn, op, &pbase, &nbase );
	if ( rc != LDAP_SUCCESS ) {
		goto return_results;
	}

#ifdef LDAP_SLAPI
	attrs = anlist2charray( an );
	pb = initSearchPlugin( be, conn, op, &pbase,
		scope, deref, sizelimit,
		timelimit, filter, &fstr, attrs, attrsonly,
		manageDSAit );
	rc = doPreSearchPluginFNs( be, pb );
	if ( rc != LDAP_SUCCESS ) {
		goto return_results;
	}

	doSearchRewriteFNs( be, pb, &filter, &fstr );
#endif /* LDAP_SLAPI */

	/* actually do the search and send the result(s) */
	if ( be->be_search ) {
		(*be->be_search)( be, conn, op, &pbase, &nbase,
			scope, deref, sizelimit,
			timelimit, filter, &fstr, an, attrsonly );
	} else {
		send_ldap_result( conn, op, rc = LDAP_UNWILLING_TO_PERFORM,
			NULL, "operation not supported within namingContext",
			NULL, NULL );
	}

#ifdef LDAP_SLAPI
	doPostSearchPluginFNs( be, pb );
#endif /* LDAP_SLAPI */

return_results:;

#ifdef LDAP_CLIENT_UPDATE
	if ( ( op->o_clientupdate_type & SLAP_LCUP_PERSIST ) )
		return rc;
#endif
#if defined(LDAP_CLIENT_UPDATE) && defined(LDAP_SYNC)
	else
#endif
#ifdef LDAP_SYNC
	if ( ( op->o_sync_mode & SLAP_SYNC_PERSIST ) )
		return rc;
#endif

	if( pbase.bv_val != NULL) free( pbase.bv_val );
	if( nbase.bv_val != NULL) free( nbase.bv_val );

	if( fstr.bv_val != NULL) free( fstr.bv_val );
	if( filter != NULL) filter_free( filter );
	if( an != NULL ) free( an );
#ifdef LDAP_SLAPI
	if( attrs != NULL) ch_free( attrs );
#endif /* LDAP_SLAPI */

	return rc;
}

#ifdef LDAP_SLAPI

static char **anlist2charray( AttributeName *an )
{
	char **attrs;
	int i;

	if ( an != NULL ) {
		for ( i = 0; an[i].an_name.bv_val != NULL; i++ )
			;
		attrs = (char **)ch_malloc( (i + 1) * sizeof(char *) );
		for ( i = 0; an[i].an_name.bv_val != NULL; i++ ) {
			attrs[i] = an[i].an_name.bv_val;
		}
		attrs[i] = NULL;
	} else {
		attrs = NULL;
	}

	return attrs;
}

static Slapi_PBlock *initSearchPlugin( Backend *be, Connection *conn, Operation *op,
	struct berval *base, int scope, int deref, int sizelimit,
	int timelimit, Filter *filter, struct berval *fstr,
	char **attrs, int attrsonly, int managedsait )
{
	Slapi_PBlock *pb;

	pb = op->o_pb;

	slapi_x_backend_set_pb( pb, be );
	slapi_x_connection_set_pb( pb, conn );
	slapi_x_operation_set_pb( pb, op );
	slapi_pblock_set( pb, SLAPI_SEARCH_TARGET, (void *)base->bv_val );
	slapi_pblock_set( pb, SLAPI_SEARCH_SCOPE, (void *)scope );
	slapi_pblock_set( pb, SLAPI_SEARCH_DEREF, (void *)deref );
	slapi_pblock_set( pb, SLAPI_SEARCH_SIZELIMIT, (void *)sizelimit );
	slapi_pblock_set( pb, SLAPI_SEARCH_TIMELIMIT, (void *)timelimit );
	slapi_pblock_set( pb, SLAPI_SEARCH_FILTER, (void *)filter );
	slapi_pblock_set( pb, SLAPI_SEARCH_STRFILTER, (void *)fstr->bv_val );
	slapi_pblock_set( pb, SLAPI_SEARCH_ATTRS, (void *)attrs );
	slapi_pblock_set( pb, SLAPI_SEARCH_ATTRSONLY, (void *)attrsonly );
	slapi_pblock_set( pb, SLAPI_MANAGEDSAIT, (void *)managedsait );

	return pb;
}

static int doPreSearchPluginFNs( Backend *be, Slapi_PBlock *pb )
{
	int rc;

	rc = doPluginFNs( be, SLAPI_PLUGIN_PRE_SEARCH_FN, pb );
	if ( rc != 0 ) {
		/*
		 * A preoperation plugin failure will abort the
		 * entire operation.
		 */
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, "doPreSearchPluginFNs: search preoperation plugin "
				"returned %d\n", rc, 0, 0 );
#else
		Debug(LDAP_DEBUG_TRACE, "doPreSearchPluginFNs: search preoperation plugin "
				"returned %d.\n", rc, 0, 0);
#endif
		if ( slapi_pblock_get( pb, SLAPI_RESULT_CODE, (void *)&rc ) != 0)
			rc = LDAP_OTHER;
	} else {
		rc = LDAP_SUCCESS;
	}

	return rc;
}

static int doSearchRewriteFNs( Backend *be, Slapi_PBlock *pb, Filter **filter, struct berval *fstr )
{
	if ( doPluginFNs( be, SLAPI_PLUGIN_COMPUTE_SEARCH_REWRITER_FN, pb ) == 0 ) {
		/*
		 * The plugin can set the SLAPI_SEARCH_FILTER.
		 * SLAPI_SEARCH_STRFILER is not normative.
		 */
		slapi_pblock_get( pb, SLAPI_SEARCH_FILTER, (void *)filter);
		ch_free( fstr->bv_val );
		filter2bv( *filter, fstr );
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ARGS, 
			"doSearchRewriteFNs: after compute_rewrite_search filter: %s\n", 
			fstr->bv_len ? fstr->bv_val : "empty", 0, 0 );
#else
		Debug( LDAP_DEBUG_ARGS, "    after compute_rewrite_search filter: %s\n",
			fstr->bv_len ? fstr->bv_val : "empty", 0, 0 );
#endif
	}

	return LDAP_SUCCESS;
}

static void doPostSearchPluginFNs( Backend *be, Slapi_PBlock *pb )
{
	if ( doPluginFNs( be, SLAPI_PLUGIN_POST_SEARCH_FN, pb ) != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, "doPostSearchPluginFNs: search postoperation plugins "
				"failed\n", 0, 0, 0 );
#else
		Debug(LDAP_DEBUG_TRACE, "doPostSearchPluginFNs: search postoperation plugins "
				"failed.\n", 0, 0, 0);
#endif
	}
}

void dummy(void)
{
	/*
	 * XXX slapi_search_internal() was no getting pulled
	 * in; all manner of linker flags failed to link it.
	 * FIXME
	 */
	slapi_search_internal( NULL, 0, NULL, NULL, NULL, 0 );
}
#endif /* LDAP_SLAPI */

