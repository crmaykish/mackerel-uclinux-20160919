/* $OpenLDAP: pkg/ldap/include/ldap_int_thread.h,v 1.10.2.1 2003/03/03 17:10:03 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, Redwood City, California, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.  A copy of this license is available at
 * http://www.OpenLDAP.org/license.html or in file LICENSE in the
 * top-level directory of the distribution.
 */
/* ldap_int_thread.h - ldap internal thread wrappers header file */

#ifndef _LDAP_INT_THREAD_H
#define _LDAP_INT_THREAD_H

#if defined( HAVE_PTHREADS )
/**********************************
 *                                *
 * definitions for POSIX Threads  *
 *                                *
 **********************************/

#include <pthread.h>
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

LDAP_BEGIN_DECL

typedef pthread_t		ldap_int_thread_t;
typedef pthread_mutex_t		ldap_int_thread_mutex_t;
typedef pthread_cond_t		ldap_int_thread_cond_t;

#if defined( _POSIX_REENTRANT_FUNCTIONS ) || \
	defined( _POSIX_THREAD_SAFE_FUNCTIONS ) || \
	defined( _POSIX_THREADSAFE_FUNCTIONS )
#define HAVE_REENTRANT_FUNCTIONS 1
#endif

#if defined( HAVE_PTHREAD_GETCONCURRENCY ) || \
	defined( HAVE_THR_GETCONCURRENCY )
#define LDAP_THREAD_HAVE_GETCONCURRENCY 1
#endif

#if defined( HAVE_PTHREAD_SETCONCURRENCY ) || \
	defined( HAVE_THR_SETCONCURRENCY )
#define LDAP_THREAD_HAVE_SETCONCURRENCY 1
#endif

#if 0 && defined( HAVE_PTHREAD_RWLOCK_DESTROY )
#define LDAP_THREAD_HAVE_RDWR 1
typedef pthread_rwlock_t ldap_pvt_thread_rdwr_t;
#endif

LDAP_END_DECL

#elif defined ( HAVE_MACH_CTHREADS )
/**********************************
 *                                *
 * definitions for Mach CThreads  *
 *                                *
 **********************************/

#if defined( HAVE_MACH_CTHREADS_H )
#	include <mach/cthreads.h>
#elif defined( HAVE_CTHREAD_H
#	include <cthreads.h>
#endif

LDAP_BEGIN_DECL

typedef cthread_t		ldap_int_thread_t;
typedef struct mutex		ldap_int_thread_mutex_t;
typedef struct condition	ldap_int_thread_cond_t;

LDAP_END_DECL

#elif defined( HAVE_GNU_PTH )
/***********************************
 *                                 *
 * thread definitions for GNU Pth  *
 *                                 *
 ***********************************/

#define PTH_SYSCALL_SOFT 1
#include <pth.h>

LDAP_BEGIN_DECL

typedef pth_t		ldap_int_thread_t;
typedef pth_mutex_t	ldap_int_thread_mutex_t;
typedef pth_cond_t	ldap_int_thread_cond_t;

#if 0
#define LDAP_THREAD_HAVE_RDWR 1
typedef pth_rwlock_t ldap_pvt_thread_rdwr_t;
#endif

LDAP_END_DECL

#elif defined( HAVE_THR )
/********************************************
 *                                          *
 * thread definitions for Solaris LWP (THR) *
 *                                          *
 ********************************************/

#include <thread.h>
#include <synch.h>

LDAP_BEGIN_DECL

typedef thread_t		ldap_int_thread_t;
typedef mutex_t			ldap_int_thread_mutex_t;
typedef cond_t			ldap_int_thread_cond_t;

#define HAVE_REENTRANT_FUNCTIONS 1

#ifdef HAVE_THR_GETCONCURRENCY
#define LDAP_THREAD_HAVE_GETCONCURRENCY 1
#endif
#ifdef HAVE_THR_SETCONCURRENCY
#define LDAP_THREAD_HAVE_SETCONCURRENCY 1
#endif

LDAP_END_DECL

#elif defined( HAVE_LWP )
/*************************************
 *                                   *
 * thread definitions for SunOS LWP  *
 *                                   *
 *************************************/

#include <lwp/lwp.h>
#include <lwp/stackdep.h>
#define LDAP_THREAD_HAVE_SLEEP 1

LDAP_BEGIN_DECL

typedef thread_t		ldap_int_thread_t;
typedef mon_t			ldap_int_thread_mutex_t;
struct ldap_int_thread_lwp_cv {
	int		lcv_created;
	cv_t		lcv_cv;
};
typedef struct ldap_int_thread_lwp_cv ldap_int_thread_cond_t;

#define HAVE_REENTRANT_FUNCTIONS 1

LDAP_END_DECL

#elif defined(HAVE_NT_THREADS)

#include <process.h>
#include <windows.h>

LDAP_BEGIN_DECL

typedef unsigned long	ldap_int_thread_t;
typedef HANDLE	ldap_int_thread_mutex_t;
typedef HANDLE	ldap_int_thread_cond_t;

LDAP_END_DECL

#else

/***********************************
 *                                 *
 * thread definitions for no       *
 * underlying library support      *
 *                                 *
 ***********************************/

#ifndef NO_THREADS
#define NO_THREADS 1
#endif

LDAP_BEGIN_DECL

typedef int			ldap_int_thread_t;
typedef int			ldap_int_thread_mutex_t;
typedef int			ldap_int_thread_cond_t;

#define LDAP_THREAD_HAVE_TPOOL 1
typedef int			ldap_int_thread_pool_t;

LDAP_END_DECL

#endif /* no threads support */

LDAP_BEGIN_DECL

LDAP_F(int) ldap_int_thread_initialize LDAP_P(( void ));
LDAP_F(int) ldap_int_thread_destroy LDAP_P(( void ));
LDAP_F(int) ldap_int_thread_pool_startup ( void );
LDAP_F(int) ldap_int_thread_pool_shutdown ( void );

#ifndef LDAP_THREAD_HAVE_TPOOL
typedef struct ldap_int_thread_pool_s * ldap_int_thread_pool_t;
#endif

LDAP_END_DECL

#endif /* _LDAP_INT_THREAD_H */
