/* $OpenLDAP: pkg/ldap/libraries/libldap_r/thr_thr.c,v 1.10.2.3 2003/03/03 17:10:05 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, Redwood City, California, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted only
 * as authorized by the OpenLDAP Public License.  A copy of this
 * license is available at http://www.OpenLDAP.org/license.html or
 * in file LICENSE in the top-level directory of the distribution.
 */

/* thr_thr.c - wrappers around solaris threads */

#include "portable.h"

#if defined( HAVE_THR )

#include "ldap_pvt_thread.h"

/*******************
 *                 *
 * Solaris Threads *
 *                 *
 *******************/

int
ldap_int_thread_initialize( void )
{
	return 0;
}

int
ldap_int_thread_destroy( void )
{
	return 0;
}

#ifdef LDAP_THREAD_HAVE_SETCONCURRENCY
int
ldap_pvt_thread_set_concurrency(int n)
{
	return thr_setconcurrency( n );
}
#endif

#ifdef LDAP_THREAD_HAVE_GETCONCURRENCY
int
ldap_pvt_thread_get_concurrency(void)
{
	return thr_getconcurrency();
}
#endif

int 
ldap_pvt_thread_create( ldap_pvt_thread_t * thread, 
	int detach,
	void *(*start_routine)( void *),
	void *arg)
{
	return( thr_create( NULL, LDAP_PVT_THREAD_STACK_SIZE, start_routine,
		arg, detach ? THR_DETACHED : 0, thread ) );
}

void 
ldap_pvt_thread_exit( void *retval )
{
	thr_exit( NULL );
}

int ldap_pvt_thread_join( ldap_pvt_thread_t thread, void **thread_return )
{
	thr_join( thread, NULL, thread_return );
	return 0;
}

int 
ldap_pvt_thread_kill( ldap_pvt_thread_t thread, int signo )
{
	thr_kill( thread, signo );
	return 0;
}
	
int 
ldap_pvt_thread_yield( void )
{
	thr_yield();
	return 0;
}

int 
ldap_pvt_thread_cond_init( ldap_pvt_thread_cond_t *cond )
{
	return( cond_init( cond, USYNC_THREAD, NULL ) );
}

int 
ldap_pvt_thread_cond_signal( ldap_pvt_thread_cond_t *cond )
{
	return( cond_signal( cond ) );
}

int
ldap_pvt_thread_cond_broadcast( ldap_pvt_thread_cond_t *cv )
{
	return( cond_broadcast( cv ) );
}

int 
ldap_pvt_thread_cond_wait( ldap_pvt_thread_cond_t *cond, 
	ldap_pvt_thread_mutex_t *mutex )
{
	return( cond_wait( cond, mutex ) );
}

int
ldap_pvt_thread_cond_destroy( ldap_pvt_thread_cond_t *cv )
{
	return( cond_destroy( cv ) );
}

int 
ldap_pvt_thread_mutex_init( ldap_pvt_thread_mutex_t *mutex )
{
	return( mutex_init( mutex, USYNC_THREAD, NULL ) );
}

int 
ldap_pvt_thread_mutex_destroy( ldap_pvt_thread_mutex_t *mutex )
{
	return( mutex_destroy( mutex ) );
}

int 
ldap_pvt_thread_mutex_lock( ldap_pvt_thread_mutex_t *mutex )
{
	return( mutex_lock( mutex ) );
}

int 
ldap_pvt_thread_mutex_unlock( ldap_pvt_thread_mutex_t *mutex )
{
	return( mutex_unlock( mutex ) );
}

int
ldap_pvt_thread_mutex_trylock( ldap_pvt_thread_mutex_t *mp )
{
	return( mutex_trylock( mp ) );
}

ldap_pvt_thread_t
ldap_pvt_thread_self( void )
{
	return thr_self();
}

#endif /* HAVE_THR */
