/* bind.c - ldap backend bind function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldap/bind.c,v 1.24.2.9 2003/05/07 22:29:11 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* This is an altered version */
/*
 * Copyright 1999, Howard Chu, All rights reserved. <hyc@highlandsun.com>
 * 
 * Permission is granted to anyone to use this software for any purpose
 * on any computer system, and to alter it and redistribute it, subject
 * to the following restrictions:
 * 
 * 1. The author is not responsible for the consequences of use of this
 *    software, no matter how awful, even if they arise from flaws in it.
 * 
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Since few users ever read sources,
 *    credits should appear in the documentation.
 * 
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.  Since few users
 *    ever read sources, credits should appear in the documentation.
 * 
 * 4. This notice may not be removed or altered.
 *
 *
 *
 * Copyright 2000, Pierangelo Masarati, All rights reserved. <ando@sys-net.it>
 * 
 * This software is being modified by Pierangelo Masarati.
 * The previously reported conditions apply to the modified code as well.
 * Changes in the original code are highlighted where required.
 * Credits for the original code go to the author, Howard Chu.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>


#define AVL_INTERNAL
#include "slap.h"
#include "back-ldap.h"

#define PRINT_CONNTREE 0

static LDAP_REBIND_PROC	ldap_back_rebind;

int
ldap_back_bind(
    Backend		*be,
    Connection		*conn,
    Operation		*op,
    struct berval	*dn,
    struct berval	*ndn,
    int			method,
    struct berval	*cred,
    struct berval	*edn
)
{
	struct ldapinfo	*li = (struct ldapinfo *) be->be_private;
	struct ldapconn *lc;

	struct berval mdn = { 0, NULL };
	int rc = 0;

	lc = ldap_back_getconn(li, conn, op);
	if ( !lc ) {
		return( -1 );
	}

	if ( op->o_ctrls ) {
		if ( ldap_set_option( lc->ld, LDAP_OPT_SERVER_CONTROLS,
					op->o_ctrls ) != LDAP_SUCCESS ) {
			ldap_back_op_result( lc, op );
			return( -1 );
		}
	}
	
	/*
	 * Rewrite the bind dn if needed
	 */
#ifdef ENABLE_REWRITE
	switch ( rewrite_session( li->rwinfo, "bindDn", dn->bv_val, conn, &mdn.bv_val ) ) {
	case REWRITE_REGEXEC_OK:
		if ( mdn.bv_val == NULL ) {
			mdn.bv_val = ( char * )dn->bv_val;
		}
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDAP, DETAIL1, 
			"[rw] bindDn: \"%s\" -> \"%s\"\n", dn->bv_val, mdn.bv_val, 0 );
#else /* !NEW_LOGGING */
		Debug( LDAP_DEBUG_ARGS, "rw> bindDn: \"%s\" -> \"%s\"\n%s",
				dn->bv_val, mdn.bv_val, "" );
#endif /* !NEW_LOGGING */
		break;
		
	case REWRITE_REGEXEC_UNWILLING:
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM,
				NULL, "Operation not allowed", NULL, NULL );
		return( -1 );

	case REWRITE_REGEXEC_ERR:
		send_ldap_result( conn, op, LDAP_OTHER,
				NULL, "Rewrite error", NULL, NULL );
		return( -1 );
	}
#else /* !ENABLE_REWRITE */
	ldap_back_dn_massage( li, dn, &mdn, 0, 1 );
#endif /* !ENABLE_REWRITE */

	rc = ldap_bind_s(lc->ld, mdn.bv_val, cred->bv_val, method);
	if (rc != LDAP_SUCCESS) {
		rc = ldap_back_op_result( lc, op );
	} else {
		lc->bound = 1;
	}

	if ( li->savecred ) {
		if ( lc->cred.bv_val ) {
			memset( lc->cred.bv_val, 0, lc->cred.bv_len );
			ch_free( lc->cred.bv_val );
		}
		ber_dupbv( &lc->cred, cred );
		ldap_set_rebind_proc( lc->ld, ldap_back_rebind, lc );
	}

	if ( lc->bound_dn.bv_val )
		ch_free( lc->bound_dn.bv_val );
	if ( mdn.bv_val != dn->bv_val ) {
		lc->bound_dn = mdn;
	} else {
		ber_dupbv( &lc->bound_dn, dn );
	}
	
	return( rc );
}

/*
 * ldap_back_conn_cmp
 *
 * compares two struct ldapconn based on the value of the conn pointer;
 * used by avl stuff
 */
int
ldap_back_conn_cmp(
	const void *c1,
	const void *c2
	)
{
	const struct ldapconn *lc1 = (const struct ldapconn *)c1;
	const struct ldapconn *lc2 = (const struct ldapconn *)c2;
	
	return SLAP_PTRCMP(lc1->conn, lc2->conn);
}

/*
 * ldap_back_conn_dup
 *
 * returns -1 in case a duplicate struct ldapconn has been inserted;
 * used by avl stuff
 */
int
ldap_back_conn_dup(
	void *c1,
	void *c2
	)
{
	struct ldapconn *lc1 = (struct ldapconn *)c1;
	struct ldapconn *lc2 = (struct ldapconn *)c2;

	return( ( lc1->conn == lc2->conn ) ? -1 : 0 );
}

#if PRINT_CONNTREE > 0
static void ravl_print( Avlnode *root, int depth )
{
	int     i;
	
	if ( root == 0 )
		return;
	
	ravl_print( root->avl_right, depth+1 );
	
	for ( i = 0; i < depth; i++ )
		printf( "   " );

	printf( "c(%ld) %d\n", ((struct ldapconn *) root->avl_data)->conn->c_connid, root->avl_bf );
	
	ravl_print( root->avl_left, depth+1 );
}

static void myprint( Avlnode *root )
{
	printf( "********\n" );
	
	if ( root == 0 )
		printf( "\tNULL\n" );

	else
		ravl_print( root, 0 );
	
	printf( "********\n" );
}
#endif /* PRINT_CONNTREE */

struct ldapconn *
ldap_back_getconn(struct ldapinfo *li, Connection *conn, Operation *op)
{
	struct ldapconn *lc, lc_curr;
	LDAP *ld;

	/* Searches for a ldapconn in the avl tree */
	lc_curr.conn = conn;
	ldap_pvt_thread_mutex_lock( &li->conn_mutex );
	lc = (struct ldapconn *)avl_find( li->conntree, 
		(caddr_t)&lc_curr, ldap_back_conn_cmp );
	ldap_pvt_thread_mutex_unlock( &li->conn_mutex );

	/* Looks like we didn't get a bind. Open a new session... */
	if (!lc) {
		int vers = conn->c_protocol;
		int err = ldap_initialize(&ld, li->url);
		
		if (err != LDAP_SUCCESS) {
			err = ldap_back_map_result(err);
			send_ldap_result( conn, op, err,
				NULL, "ldap_initialize() failed", NULL, NULL );
			return( NULL );
		}
		/* Set LDAP version. This will always succeed: If the client
		 * bound with a particular version, then so can we.
		 */
		ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &vers);

		lc = (struct ldapconn *)ch_malloc(sizeof(struct ldapconn));
		lc->conn = conn;
		lc->ld = ld;

		lc->cred.bv_len = 0;
		lc->cred.bv_val = NULL;

#ifdef ENABLE_REWRITE
		/*
		 * Sets a cookie for the rewrite session
		 */
		( void )rewrite_session_init( li->rwinfo, conn );
#endif /* ENABLE_REWRITE */

		if ( lc->conn->c_dn.bv_len != 0 ) {
			
			/*
			 * Rewrite the bind dn if needed
			 */
#ifdef ENABLE_REWRITE			
			lc->bound_dn.bv_val = NULL;
			lc->bound_dn.bv_len = 0;
			switch ( rewrite_session( li->rwinfo, "bindDn",
						lc->conn->c_dn.bv_val, conn,
						&lc->bound_dn.bv_val ) ) {
			case REWRITE_REGEXEC_OK:
				if ( lc->bound_dn.bv_val == NULL ) {
					ber_dupbv( &lc->bound_dn,
							&lc->conn->c_dn );
				}
#ifdef NEW_LOGGING
				LDAP_LOG( BACK_LDAP, DETAIL1, 
						"[rw] bindDn: \"%s\" ->" 
						" \"%s\"\n%s",
						lc->conn->c_dn.bv_val, 
						lc->bound_dn.bv_val, "" );
#else /* !NEW_LOGGING */
				Debug( LDAP_DEBUG_ARGS,
					       	"rw> bindDn: \"%s\" ->"
						" \"%s\"\n%s",
						lc->conn->c_dn.bv_val,
						lc->bound_dn.bv_val, "" );
#endif /* !NEW_LOGGING */
				break;
				
			case REWRITE_REGEXEC_UNWILLING:
				send_ldap_result( conn, op,
						LDAP_UNWILLING_TO_PERFORM,
						NULL, "Operation not allowed",
						NULL, NULL );
				return( NULL );
				
			case REWRITE_REGEXEC_ERR:
				send_ldap_result( conn, op,
						LDAP_OTHER,
						NULL, "Rewrite error",
						NULL, NULL );
				return( NULL );
			}

#else /* !ENABLE_REWRITE */
			struct berval bv;
			ldap_back_dn_massage( li, &lc->conn->c_dn, &bv, 0, 1 );
			if ( bv.bv_val == lc->conn->c_dn.bv_val ) {
				ber_dupbv( &lc->bound_dn, &bv );
			} else {
				lc->bound_dn = bv;
			}
#endif /* !ENABLE_REWRITE */

		} else {
			lc->bound_dn.bv_val = NULL;
			lc->bound_dn.bv_len = 0;
		}
		lc->bound = 0;

		/* Inserts the newly created ldapconn in the avl tree */
		ldap_pvt_thread_mutex_lock( &li->conn_mutex );
		err = avl_insert( &li->conntree, (caddr_t)lc,
			ldap_back_conn_cmp, ldap_back_conn_dup );

#if PRINT_CONNTREE > 0
		myprint( li->conntree );
#endif /* PRINT_CONNTREE */
		
		ldap_pvt_thread_mutex_unlock( &li->conn_mutex );

#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDAP, INFO, 
			"ldap_back_getconn: conn %ld inserted\n", lc->conn->c_connid, 0, 0);
#else /* !NEW_LOGGING */
		Debug( LDAP_DEBUG_TRACE,
			"=>ldap_back_getconn: conn %ld inserted\n%s%s",
			lc->conn->c_connid, "", "" );
#endif /* !NEW_LOGGING */
		
		/* Err could be -1 in case a duplicate ldapconn is inserted */
		if ( err != 0 ) {
			send_ldap_result( conn, op, LDAP_OTHER,
			NULL, "internal server error", NULL, NULL );
			/* better destroy the ldapconn struct? */
			return( NULL );
		}
	} else {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDAP, INFO, 
			"ldap_back_getconn: conn %ld inserted\n", 
			lc->conn->c_connid, 0, 0 );
#else /* !NEW_LOGGING */
		Debug( LDAP_DEBUG_TRACE,
			"=>ldap_back_getconn: conn %ld fetched%s%s\n",
			lc->conn->c_connid, "", "" );
#endif /* !NEW_LOGGING */
	}
	
	return( lc );
}

/*
 * ldap_back_dobind
 *
 * Note: as the check for the value of lc->bound was already here, I removed
 * it from all the callers, and I made the function return the flag, so
 * it can be used to simplify the check.
 */
int
ldap_back_dobind( struct ldapconn *lc, Operation *op )
{	
	if ( op->o_ctrls ) {
		if ( ldap_set_option( lc->ld, LDAP_OPT_SERVER_CONTROLS,
				op->o_ctrls ) != LDAP_SUCCESS ) {
			ldap_back_op_result( lc, op );
			return( 0 );
		}
	}
	
	if ( lc->bound ) {
		return( lc->bound );
	}

	if ( ldap_bind_s( lc->ld, lc->bound_dn.bv_val, lc->cred.bv_val, 
				LDAP_AUTH_SIMPLE ) != LDAP_SUCCESS ) {
		ldap_back_op_result( lc, op );
		return( 0 );
	} /* else */
	return( lc->bound = 1 );
}

/*
 * ldap_back_rebind
 *
 * This is a callback used for chasing referrals using the same
 * credentials as the original user on this session.
 */
static int 
ldap_back_rebind( LDAP *ld, LDAP_CONST char *url, ber_tag_t request,
	ber_int_t msgid, void *params )
{
	struct ldapconn *lc = params;

	return ldap_bind_s( ld, lc->bound_dn.bv_val, lc->cred.bv_val, LDAP_AUTH_SIMPLE );
}

/* Map API errors to protocol errors... */

int
ldap_back_map_result(int err)
{
	switch(err)
	{
	case LDAP_SERVER_DOWN:
		return LDAP_UNAVAILABLE;
	case LDAP_LOCAL_ERROR:
		return LDAP_OTHER;
	case LDAP_ENCODING_ERROR:
	case LDAP_DECODING_ERROR:
		return LDAP_PROTOCOL_ERROR;
	case LDAP_TIMEOUT:
		return LDAP_UNAVAILABLE;
	case LDAP_AUTH_UNKNOWN:
		return LDAP_AUTH_METHOD_NOT_SUPPORTED;
	case LDAP_FILTER_ERROR:
		return LDAP_OTHER;
	case LDAP_USER_CANCELLED:
		return LDAP_OTHER;
	case LDAP_PARAM_ERROR:
		return LDAP_PROTOCOL_ERROR;
	case LDAP_NO_MEMORY:
		return LDAP_OTHER;
	case LDAP_CONNECT_ERROR:
		return LDAP_UNAVAILABLE;
	case LDAP_NOT_SUPPORTED:
		return LDAP_UNWILLING_TO_PERFORM;
	case LDAP_CONTROL_NOT_FOUND:
		return LDAP_PROTOCOL_ERROR;
	case LDAP_NO_RESULTS_RETURNED:
		return LDAP_NO_SUCH_OBJECT;
	case LDAP_MORE_RESULTS_TO_RETURN:
		return LDAP_OTHER;
	case LDAP_CLIENT_LOOP:
	case LDAP_REFERRAL_LIMIT_EXCEEDED:
		return LDAP_LOOP_DETECT;
	default:
		if LDAP_API_ERROR(err)
			return LDAP_OTHER;
		else
			return err;
	}
}

int
ldap_back_op_result(struct ldapconn *lc, Operation *op)
{
	int err = LDAP_SUCCESS;
	char *msg = NULL;
	char *match = NULL;

	ldap_get_option(lc->ld, LDAP_OPT_ERROR_NUMBER, &err);
	ldap_get_option(lc->ld, LDAP_OPT_ERROR_STRING, &msg);
	ldap_get_option(lc->ld, LDAP_OPT_MATCHED_DN, &match);
	err = ldap_back_map_result(err);

#ifdef ENABLE_REWRITE
	
	/*
	 * FIXME: need rewrite info for match; mmmh ...
	 */
	send_ldap_result( lc->conn, op, err, match, msg, NULL, NULL );
	/* better test the pointers before freeing? */
	if ( match ) {
		free( match );
	}

#else /* !ENABLE_REWRITE */

	send_ldap_result( lc->conn, op, err, match, msg, NULL, NULL );
	/* better test the pointers before freeing? */
	if ( match ) {
		free( match );
	}

#endif /* !ENABLE_REWRITE */

	if ( msg ) free( msg );
	return( (err==LDAP_SUCCESS) ? 0 : -1 );
}

