/* operation.c - routines to deal with pending ldap operations */
/* $OpenLDAP: pkg/ldap/servers/slapd/operation.c,v 1.25.2.7 2003/03/29 15:45:43 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

#ifdef LDAP_SLAPI
#include "slapi.h"
#endif

static ldap_pvt_thread_mutex_t	slap_op_mutex;
static LDAP_STAILQ_HEAD(s_o, slap_op)	slap_free_ops;

void slap_op_init(void)
{
	ldap_pvt_thread_mutex_init( &slap_op_mutex );
	LDAP_STAILQ_INIT(&slap_free_ops);
}

void slap_op_destroy(void)
{
	Operation *o;

	while ( (o = LDAP_STAILQ_FIRST( &slap_free_ops )) != NULL) {
		LDAP_STAILQ_REMOVE_HEAD( &slap_free_ops, o_next );
		LDAP_STAILQ_NEXT(o, o_next) = NULL;
		ch_free( o );
	}
	ldap_pvt_thread_mutex_destroy( &slap_op_mutex );
}

void
slap_op_free( Operation *op )
{
	assert( LDAP_STAILQ_NEXT(op, o_next) == NULL );

	if ( op->o_ber != NULL ) {
		ber_free( op->o_ber, 1 );
	}
	if ( op->o_dn.bv_val != NULL ) {
		free( op->o_dn.bv_val );
	}
	if ( op->o_ndn.bv_val != NULL ) {
		free( op->o_ndn.bv_val );
	}
	if ( op->o_authmech.bv_val != NULL ) {
		free( op->o_authmech.bv_val );
	}
	if ( op->o_ctrls != NULL ) {
		ldap_controls_free( op->o_ctrls );
	}

#ifdef LDAP_CONNECTIONLESS
	if ( op->o_res_ber != NULL ) {
		ber_free( op->o_res_ber, 1 );
	}
#endif
#ifdef LDAP_CLIENT_UPDATE
	if ( op->o_clientupdate_state.bv_val != NULL ) {
		free( op->o_clientupdate_state.bv_val );
	}
#endif
#ifdef LDAP_SYNC
	if ( op->o_sync_state.bv_val != NULL ) {
		free( op->o_sync_state.bv_val );
	}
#endif

#if defined( LDAP_SLAPI )
	if ( op->o_pb != NULL ) {
		slapi_pblock_destroy( (Slapi_PBlock *)op->o_pb );
	}
#endif /* defined( LDAP_SLAPI ) */

	memset( op, 0, sizeof(Operation) );
	ldap_pvt_thread_mutex_lock( &slap_op_mutex );
	LDAP_STAILQ_INSERT_HEAD( &slap_free_ops, op, o_next );
	ldap_pvt_thread_mutex_unlock( &slap_op_mutex );
}

Operation *
slap_op_alloc(
    BerElement		*ber,
    ber_int_t	msgid,
    ber_tag_t	tag,
    ber_int_t	id
)
{
	Operation	*op;

	ldap_pvt_thread_mutex_lock( &slap_op_mutex );
	if (op = LDAP_STAILQ_FIRST( &slap_free_ops )) {
		LDAP_STAILQ_REMOVE_HEAD( &slap_free_ops, o_next );
	}
	ldap_pvt_thread_mutex_unlock( &slap_op_mutex );

	if (!op)
		op = (Operation *) ch_calloc( 1, sizeof(Operation) );

	op->o_ber = ber;
	op->o_msgid = msgid;
	op->o_tag = tag;

	op->o_time = slap_get_time();
	op->o_opid = id;
#ifdef LDAP_CONNECTIONLESS
	op->o_res_ber = NULL;
#endif

#if defined( LDAP_SLAPI )
	op->o_pb = slapi_pblock_new();
#endif /* defined( LDAP_SLAPI ) */

	return( op );
}
