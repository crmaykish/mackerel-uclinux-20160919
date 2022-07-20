/* unbind.c - ldap backend unbind function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldap/unbind.c,v 1.12.2.4 2003/02/09 16:31:38 kurt Exp $ */
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

#include "slap.h"
#include "back-ldap.h"

int
ldap_back_conn_destroy(
    Backend		*be,
    Connection		*conn
)
{
	struct ldapinfo	*li = (struct ldapinfo *) be->be_private;
	struct ldapconn *lc, lc_curr;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDAP, INFO,
		"ldap_back_conn_destroy: fetching conn %ld\n", conn->c_connid, 0, 0 );
#else /* !NEW_LOGGING */
	Debug( LDAP_DEBUG_TRACE,
		"=>ldap_back_conn_destroy: fetching conn %ld\n",
		conn->c_connid, 0, 0 );
#endif /* !NEW_LOGGING */

	lc_curr.conn = conn;
	
	ldap_pvt_thread_mutex_lock( &li->conn_mutex );
	lc = avl_delete( &li->conntree, (caddr_t)&lc_curr, ldap_back_conn_cmp );
	ldap_pvt_thread_mutex_unlock( &li->conn_mutex );

	if (lc) {
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDAP, DETAIL1, 
			"ldap_back_conn_destroy: destroying conn %ld\n", 
			conn->c_connid, 0, 0 );
#else /* !NEW_LOGGING */
		Debug( LDAP_DEBUG_TRACE,
			"=>ldap_back_conn_destroy: destroying conn %ld\n",
			lc->conn->c_connid, 0, 0 );
#endif

#ifdef ENABLE_REWRITE
		/*
		 * Cleanup rewrite session
		 */
		rewrite_session_delete( li->rwinfo, conn );
#endif /* ENABLE_REWRITE */

		/*
		 * Needs a test because the handler may be corrupted,
		 * and calling ldap_unbind on a corrupted header results
		 * in a segmentation fault
		 */
		ldap_unbind(lc->ld);
		if ( lc->bound_dn.bv_val ) {
			ch_free( lc->bound_dn.bv_val );
		}
		if ( lc->cred.bv_val ) {
			ch_free( lc->cred.bv_val );
		}
		ch_free( lc );
	}

	/* no response to unbind */

	return 0;
}
