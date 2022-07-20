/* $OpenLDAP: pkg/ldap/libraries/libldap_r/tpool.c,v 1.15.2.7 2003/03/27 03:06:20 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, Redwood City, California, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted only
 * as authorized by the OpenLDAP Public License.  A copy of this
 * license is available at http://www.OpenLDAP.org/license.html or
 * in file LICENSE in the top-level directory of the distribution.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdarg.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/errno.h>

#include "ldap-int.h"
#include "ldap_pvt_thread.h"
#include "ldap_queue.h"

#ifndef LDAP_THREAD_HAVE_TPOOL

enum ldap_int_thread_pool_state {
	LDAP_INT_THREAD_POOL_RUNNING,
	LDAP_INT_THREAD_POOL_FINISHING,
	LDAP_INT_THREAD_POOL_STOPPING
};

typedef struct ldap_int_thread_key_s {
	void *ltk_key;
	void *ltk_data;
	ldap_pvt_thread_pool_keyfree_t *ltk_free;
} ldap_int_thread_key_t;

/* Max number of thread-specific keys we store per thread.
 * We don't expect to use many...
 */
#define	MAXKEYS	32

typedef struct ldap_int_thread_ctx_s {
	union {
	LDAP_STAILQ_ENTRY(ldap_int_thread_ctx_s) q;
	LDAP_SLIST_ENTRY(ldap_int_thread_ctx_s) l;
	LDAP_SLIST_ENTRY(ldap_int_thread_ctx_s) al;
	} ltc_next;
	ldap_pvt_thread_start_t *ltc_start_routine;
	void *ltc_arg;
	ldap_pvt_thread_t ltc_thread_id;
	ldap_int_thread_key_t *ltc_key;
} ldap_int_thread_ctx_t;

struct ldap_int_thread_pool_s {
	LDAP_STAILQ_ENTRY(ldap_int_thread_pool_s) ltp_next;
	ldap_pvt_thread_mutex_t ltp_mutex;
	ldap_pvt_thread_cond_t ltp_cond;
	LDAP_STAILQ_HEAD(tcq, ldap_int_thread_ctx_s) ltp_pending_list;
	LDAP_SLIST_HEAD(tcl, ldap_int_thread_ctx_s) ltp_free_list;
	LDAP_SLIST_HEAD(tclq, ldap_int_thread_ctx_s) ltp_active_list;
	long ltp_state;
	long ltp_max_count;
	long ltp_max_pending;
	long ltp_pending_count;
	long ltp_active_count;
	long ltp_open_count;
	long ltp_starting;
};

static LDAP_STAILQ_HEAD(tpq, ldap_int_thread_pool_s)
	ldap_int_thread_pool_list =
	LDAP_STAILQ_HEAD_INITIALIZER(ldap_int_thread_pool_list);

static ldap_pvt_thread_mutex_t ldap_pvt_thread_pool_mutex;

static void *ldap_int_thread_pool_wrapper( void *pool );

int
ldap_int_thread_pool_startup ( void )
{
	return ldap_pvt_thread_mutex_init(&ldap_pvt_thread_pool_mutex);
}

int
ldap_int_thread_pool_shutdown ( void )
{
	struct ldap_int_thread_pool_s *pool;

	while ((pool = LDAP_STAILQ_FIRST(&ldap_int_thread_pool_list)) != NULL) {
		LDAP_STAILQ_REMOVE_HEAD(&ldap_int_thread_pool_list, ltp_next);
		ldap_pvt_thread_pool_destroy( &pool, 0);
	}
	ldap_pvt_thread_mutex_destroy(&ldap_pvt_thread_pool_mutex);
	return(0);
}

int
ldap_pvt_thread_pool_init (
	ldap_pvt_thread_pool_t *tpool,
	int max_threads,
	int max_pending )
{
	ldap_pvt_thread_pool_t pool;
	int rc;

	*tpool = NULL;
	pool = (ldap_pvt_thread_pool_t) LDAP_CALLOC(1,
		sizeof(struct ldap_int_thread_pool_s));

	if (pool == NULL) return(-1);

	rc = ldap_pvt_thread_mutex_init(&pool->ltp_mutex);
	if (rc != 0)
		return(rc);
	rc = ldap_pvt_thread_cond_init(&pool->ltp_cond);
	if (rc != 0)
		return(rc);
	pool->ltp_state = LDAP_INT_THREAD_POOL_RUNNING;
	pool->ltp_max_count = max_threads;
	pool->ltp_max_pending = max_pending;
	LDAP_STAILQ_INIT(&pool->ltp_pending_list);
	LDAP_SLIST_INIT(&pool->ltp_free_list);
	LDAP_SLIST_INIT(&pool->ltp_active_list);
	ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
	LDAP_STAILQ_INSERT_TAIL(&ldap_int_thread_pool_list, pool, ltp_next);
	ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);

#if 0
	/* THIS WILL NOT WORK on some systems.  If the process
	 * forks after starting a thread, there is no guarantee
	 * that the thread will survive the fork.  For example,
	 * slapd forks in order to daemonize, and does so after
	 * calling ldap_pvt_thread_pool_init.  On some systems,
	 * this initial thread does not run in the child process,
	 * but ltp_open_count == 1, so two things happen: 
	 * 1) the first client connection fails, and 2) when
	 * slapd is kill'ed, it never terminates since it waits
	 * for all worker threads to exit. */

	/* start up one thread, just so there is one. no need to
	 * lock the mutex right now, since no threads are running.
	 */
	pool->ltp_open_count++;

	ldap_pvt_thread_t thr;
	rc = ldap_pvt_thread_create( &thr, 1, ldap_int_thread_pool_wrapper, pool );

	if( rc != 0) {
		/* couldn't start one?  then don't start any */
		ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
		LDAP_STAILQ_REMOVE(ldap_int_thread_pool_list, pool, 
			ldap_int_thread_pool_s, ltp_next);
		ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);
		ldap_pvt_thread_cond_destroy(&pool->ltp_cond);
		ldap_pvt_thread_mutex_destroy(&pool->ltp_mutex);
		LDAP_FREE(pool);
		return(-1);
	}
#endif

	*tpool = pool;
	return(0);
}

int
ldap_pvt_thread_pool_submit (
	ldap_pvt_thread_pool_t *tpool,
	ldap_pvt_thread_start_t *start_routine, void *arg )
{
	struct ldap_int_thread_pool_s *pool;
	ldap_int_thread_ctx_t *ctx;
	int need_thread = 0;
	ldap_pvt_thread_t thr;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	if (pool->ltp_state != LDAP_INT_THREAD_POOL_RUNNING
		|| (pool->ltp_max_pending > 0
			&& pool->ltp_pending_count >= pool->ltp_max_pending))
	{
		ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
		return(-1);
	}
	ctx = LDAP_SLIST_FIRST(&pool->ltp_free_list);
	if (ctx) {
		LDAP_SLIST_REMOVE_HEAD(&pool->ltp_free_list, ltc_next.l);
	} else {
		int i;
		ctx = (ldap_int_thread_ctx_t *) LDAP_MALLOC(
			sizeof(ldap_int_thread_ctx_t));
		if (ctx == NULL) {
			ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
			return(-1);
		}
	}

	ctx->ltc_start_routine = start_routine;
	ctx->ltc_arg = arg;

	pool->ltp_pending_count++;
	LDAP_STAILQ_INSERT_TAIL(&pool->ltp_pending_list, ctx, ltc_next.q);
	ldap_pvt_thread_cond_signal(&pool->ltp_cond);
	if ((pool->ltp_open_count <= 0
#if 0
			|| pool->ltp_pending_count > 1
#endif
			|| pool->ltp_open_count == pool->ltp_active_count)
		&& (pool->ltp_max_count <= 0
			|| pool->ltp_open_count < pool->ltp_max_count))
	{
		pool->ltp_open_count++;
		pool->ltp_starting++;
		need_thread = 1;
	}
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

	if (need_thread) {
		int rc = ldap_pvt_thread_create( &thr, 1,
			ldap_int_thread_pool_wrapper, pool );
		ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
		if (rc == 0) {
			pool->ltp_starting--;
		} else {
			/* couldn't create thread.  back out of
			 * ltp_open_count and check for even worse things.
			 */
			pool->ltp_open_count--;
			pool->ltp_starting--;
			if (pool->ltp_open_count == 0) {
				/* no open threads at all?!?
				 */
				ldap_int_thread_ctx_t *ptr;
				LDAP_STAILQ_FOREACH(ptr, &pool->ltp_pending_list, ltc_next.q)
					if (ptr == ctx) break;
				if (ptr == ctx) {
					/* no open threads, context not handled, so
					 * back out of ltp_pending_count, free the context,
					 * report the error.
					 */
					LDAP_STAILQ_REMOVE(&pool->ltp_pending_list, ctx, 
						ldap_int_thread_ctx_s, ltc_next.q);
					pool->ltp_pending_count++;
					ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
					LDAP_FREE(ctx);
					return(-1);
				}
			}
			/* there is another open thread, so this
			 * context will be handled eventually.
			 * continue on and signal that the context
			 * is waiting.
			 */
		}
		ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	}

	return(0);
}

int
ldap_pvt_thread_pool_maxthreads ( ldap_pvt_thread_pool_t *tpool, int max_threads )
{
	struct ldap_int_thread_pool_s *pool;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	pool->ltp_max_count = max_threads;
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(0);
}

int
ldap_pvt_thread_pool_backload ( ldap_pvt_thread_pool_t *tpool )
{
	struct ldap_int_thread_pool_s *pool;
	int count;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL)
		return(0);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	count = pool->ltp_pending_count + pool->ltp_active_count;
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	return(count);
}

int
ldap_pvt_thread_pool_destroy ( ldap_pvt_thread_pool_t *tpool, int run_pending )
{
	struct ldap_int_thread_pool_s *pool, *pptr;
	long waiting;
	ldap_int_thread_ctx_t *ctx;

	if (tpool == NULL)
		return(-1);

	pool = *tpool;

	if (pool == NULL) return(-1);

	ldap_pvt_thread_mutex_lock(&ldap_pvt_thread_pool_mutex);
	LDAP_STAILQ_FOREACH(pptr, &ldap_int_thread_pool_list, ltp_next)
		if (pptr == pool) break;
	if (pptr == pool)
		LDAP_STAILQ_REMOVE(&ldap_int_thread_pool_list, pool,
			ldap_int_thread_pool_s, ltp_next);
	ldap_pvt_thread_mutex_unlock(&ldap_pvt_thread_pool_mutex);

	if (pool != pptr) return(-1);

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	pool->ltp_state = run_pending
		? LDAP_INT_THREAD_POOL_FINISHING
		: LDAP_INT_THREAD_POOL_STOPPING;

	ldap_pvt_thread_cond_broadcast(&pool->ltp_cond);
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

	do {
		ldap_pvt_thread_yield();
		ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
		waiting = pool->ltp_open_count;
		ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);
	} while (waiting > 0);

	while ((ctx = LDAP_STAILQ_FIRST(&pool->ltp_pending_list)) != NULL)
	{
		LDAP_STAILQ_REMOVE_HEAD(&pool->ltp_pending_list, ltc_next.q);
		LDAP_FREE(ctx);
	}

	while ((ctx = LDAP_SLIST_FIRST(&pool->ltp_free_list)) != NULL)
	{
		LDAP_SLIST_REMOVE_HEAD(&pool->ltp_free_list, ltc_next.l);
		LDAP_FREE(ctx);
	}

	ldap_pvt_thread_cond_destroy(&pool->ltp_cond);
	ldap_pvt_thread_mutex_destroy(&pool->ltp_mutex);
	LDAP_FREE(pool);
	return(0);
}

static void *
ldap_int_thread_pool_wrapper ( 
	void *xpool )
{
	struct ldap_int_thread_pool_s *pool = xpool;
	ldap_int_thread_ctx_t *ctx;
	ldap_int_thread_key_t ltc_key[MAXKEYS];
	int i;

	if (pool == NULL)
		return NULL;

	for ( i=0; i<MAXKEYS; i++ ) {
		ltc_key[i].ltk_key = NULL;
	}

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);

	while (pool->ltp_state != LDAP_INT_THREAD_POOL_STOPPING) {
		ctx = LDAP_STAILQ_FIRST(&pool->ltp_pending_list);
		if (ctx) {
			LDAP_STAILQ_REMOVE_HEAD(&pool->ltp_pending_list, ltc_next.q);
		} else {
			if (pool->ltp_state == LDAP_INT_THREAD_POOL_FINISHING)
				break;
			if (pool->ltp_max_count > 0
				&& pool->ltp_open_count > pool->ltp_max_count)
			{
				/* too many threads running (can happen if the
				 * maximum threads value is set during ongoing
				 * operation using ldap_pvt_thread_pool_maxthreads)
				 * so let this thread die.
				 */
				break;
			}

			/* we could check an idle timer here, and let the
			 * thread die if it has been inactive for a while.
			 * only die if there are other open threads (i.e.,
			 * always have at least one thread open).  the check
			 * should be like this:
			 *   if (pool->ltp_open_count > 1 && pool->ltp_starting == 0)
			 *       check timer, leave thread (break;)
			 */

			if (pool->ltp_state == LDAP_INT_THREAD_POOL_RUNNING)
				ldap_pvt_thread_cond_wait(&pool->ltp_cond, &pool->ltp_mutex);

			continue;
		}

		pool->ltp_pending_count--;

		ctx->ltc_key = ltc_key;
		ctx->ltc_thread_id = ldap_pvt_thread_self();

		LDAP_SLIST_INSERT_HEAD(&pool->ltp_active_list, ctx, ltc_next.al);
		pool->ltp_active_count++;
		ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

		ctx->ltc_start_routine(ctx, ctx->ltc_arg);
		ctx->ltc_key = NULL;

		ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
		LDAP_SLIST_REMOVE(&pool->ltp_active_list, ctx,
			ldap_int_thread_ctx_s, ltc_next.al);
		LDAP_SLIST_INSERT_HEAD(&pool->ltp_free_list, ctx, ltc_next.l);
		pool->ltp_active_count--;
		ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

		ldap_pvt_thread_yield();

		/* if we use an idle timer, here's
		 * a good place to update it
		 */

		ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	}

	for ( i=0; i<MAXKEYS && ltc_key[i].ltk_key; i++ ) {
		if (ltc_key[i].ltk_free)
			ltc_key[i].ltk_free(
				ltc_key[i].ltk_key,
				ltc_key[i].ltk_data );
	}

	pool->ltp_open_count--;
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

	ldap_pvt_thread_exit(NULL);
	return(NULL);
}

int ldap_pvt_thread_pool_getkey(
	void *xctx,
	void *key,
	void **data,
	ldap_pvt_thread_pool_keyfree_t **kfree )
{
	ldap_int_thread_ctx_t *ctx = xctx;
	int i;

	if ( !ctx || !data || !ctx->ltc_key ) return EINVAL;

	for ( i=0; i<MAXKEYS && ctx->ltc_key[i].ltk_key; i++ ) {
		if ( ctx->ltc_key[i].ltk_key == key ) {
			*data = ctx->ltc_key[i].ltk_data;
			if ( kfree ) *kfree = ctx->ltc_key[i].ltk_free;
			return 0;
		}
	}
	return ENOENT;
}

int ldap_pvt_thread_pool_setkey(
	void *xctx,
	void *key,
	void *data,
	ldap_pvt_thread_pool_keyfree_t *kfree )
{
	ldap_int_thread_ctx_t *ctx = xctx;
	int i;

	if ( !ctx || !key || !ctx->ltc_key ) return EINVAL;

	for ( i=0; i<MAXKEYS; i++ ) {
		if ( !ctx->ltc_key[i].ltk_key || ctx->ltc_key[i].ltk_key == key ) {
			ctx->ltc_key[i].ltk_key = key;
			ctx->ltc_key[i].ltk_data = data;
			ctx->ltc_key[i].ltk_free = kfree;
			return 0;
		}
	}
	return ENOMEM;
}

/*
 * This is necessary if the caller does not have access to the
 * thread context handle (for example, a slapd plugin calling
 * slapi_search_internal()). No doubt it is more efficient to
 * for the application to keep track of the thread context
 * handles itself.
 */
void *ldap_pvt_thread_pool_context( ldap_pvt_thread_pool_t *tpool )
{
	ldap_pvt_thread_pool_t pool;
	ldap_pvt_thread_t tid;
	ldap_int_thread_ctx_t *ptr;

	pool = *tpool;
	if (pool == NULL) {
		return NULL;
	}

	tid = ldap_pvt_thread_self();

	ldap_pvt_thread_mutex_lock(&pool->ltp_mutex);
	LDAP_SLIST_FOREACH(ptr, &pool->ltp_active_list, ltc_next.al)
		if (ptr != NULL && ptr->ltc_thread_id == tid) break;
	if (ptr != NULL && ptr->ltc_thread_id != tid) {
		ptr = NULL;
	}
	ldap_pvt_thread_mutex_unlock(&pool->ltp_mutex);

	return ptr;
}

#endif /* LDAP_HAVE_THREAD_POOL */
