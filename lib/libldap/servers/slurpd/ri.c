/* $OpenLDAP: pkg/ldap/servers/slurpd/ri.c,v 1.21.2.5 2003/03/03 17:10:11 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * ri.c - routines used to manipulate Ri structures.  An Ri (Replica
 * information) struct contains all information about one replica
 * instance.  The Ri struct is defined in slurp.h
 */


#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/signal.h>

#include "slurp.h"
#include "globals.h"


/* Forward references */
static int ismine LDAP_P(( Ri  *, Re  * ));
static int isnew LDAP_P(( Ri  *, Re  * ));


/*
 * Process any unhandled replication entries in the queue.
 */
static int
Ri_process(
    Ri *ri
)
{
    Rq		*rq = sglob->rq;
    Re		*re = NULL, *new_re = NULL;
    int		rc ;
    char	*errmsg;

    (void) SIGNAL( LDAP_SIGUSR1, do_nothing );
#ifdef SIGPIPE
    (void) SIGNAL( SIGPIPE, SIG_IGN );
#endif
    if ( ri == NULL ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "Ri_process: "
		"Error: ri == NULL!\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY, "Error: Ri_process: ri == NULL!\n", 0, 0, 0 );
#endif
	return -1;
    }

    /*
     * Startup code.  See if there's any work to do.  If not, wait on the
     * rq->rq_more condition variable.
     */
    rq->rq_lock( rq );
    while ( !sglob->slurpd_shutdown &&
	    (( re = rq->rq_gethead( rq )) == NULL )) {
	/* No work */
	if ( sglob->one_shot_mode ) {
	    /* give up if in one shot mode */
	    rq->rq_unlock( rq );
	    return 0;
	}
	/* wait on condition variable */
	ldap_pvt_thread_cond_wait( &rq->rq_more, &rq->rq_mutex );
    }

    /*
     * When we get here, there's work in the queue, and we have the
     * queue locked.  re should be pointing to the head of the queue.
     */
    rq->rq_unlock( rq );
    while ( !sglob->slurpd_shutdown ) {
	if ( re != NULL ) {
	    if ( !ismine( ri, re )) {
		/* The Re doesn't list my host:port */
#ifdef NEW_LOGGING
		LDAP_LOG ( SLURPD, DETAIL1, "Ri_process: "
			"Replica %s:%d, skip repl record for %s (not mine)\n",
			ri->ri_hostname, ri->ri_port, re->re_dn );
#else
		Debug( LDAP_DEBUG_TRACE,
			"Replica %s:%d, skip repl record for %s (not mine)\n",
			ri->ri_hostname, ri->ri_port, re->re_dn );
#endif
	    } else if ( !isnew( ri, re )) {
		/* This Re is older than my saved status information */
#ifdef NEW_LOGGING
		LDAP_LOG ( SLURPD, DETAIL1, "Ri_process: "
			"Replica %s:%d, skip repl record for %s (old)\n",
			ri->ri_hostname, ri->ri_port, re->re_dn );
#else
		Debug( LDAP_DEBUG_TRACE,
			"Replica %s:%d, skip repl record for %s (old)\n",
			ri->ri_hostname, ri->ri_port, re->re_dn );
#endif
	    } else {
		rc = do_ldap( ri, re, &errmsg );
		switch ( rc ) {
		case DO_LDAP_ERR_RETRYABLE:
		    ldap_pvt_thread_sleep( RETRY_SLEEP_TIME );
#ifdef NEW_LOGGING
			LDAP_LOG ( SLURPD, DETAIL1, "Ri_process: "
				"Retrying operation for DN %s on replica %s:%d\n",
			    re->re_dn, ri->ri_hostname, ri->ri_port );
#else
		    Debug( LDAP_DEBUG_ANY,
			    "Retrying operation for DN %s on replica %s:%d\n",
			    re->re_dn, ri->ri_hostname, ri->ri_port );
#endif
		    continue;
		    break;
		case DO_LDAP_ERR_FATAL: {
		    /* Non-retryable error.  Write rejection log. */
			int ld_errno = 0;
			ldap_get_option(ri->ri_ldp, LDAP_OPT_ERROR_NUMBER, &ld_errno);
		    write_reject( ri, re, ld_errno, errmsg );
		    /* Update status ... */
		    (void) sglob->st->st_update( sglob->st, ri->ri_stel, re );
		    /* ... and write to disk */
		    (void) sglob->st->st_write( sglob->st );
		    } break;
		default:
		    /* LDAP op completed ok - update status... */
		    (void) sglob->st->st_update( sglob->st, ri->ri_stel, re );
		    /* ... and write to disk */
		    (void) sglob->st->st_write( sglob->st );
		    break;
		}
	    }
	} else {
#ifdef NEW_LOGGING
		LDAP_LOG ( SLURPD, ERR, "Ri_process: "
			"Error: re is null in Ri_process\n", 0, 0, 0 );
#else
	    Debug( LDAP_DEBUG_ANY, "Error: re is null in Ri_process\n",
		    0, 0, 0 );
#endif
	}
	rq->rq_lock( rq );
	while ( !sglob->slurpd_shutdown &&
		((new_re = re->re_getnext( re )) == NULL )) {
	    if ( sglob->one_shot_mode ) {
		rq->rq_unlock( rq );
		return 0;
	    }
	    /* No work - wait on condition variable */
	    ldap_pvt_thread_cond_wait( &rq->rq_more, &rq->rq_mutex );
	}
	re->re_decrefcnt( re );
	re = new_re;
	rq->rq_unlock( rq );
	if ( sglob->slurpd_shutdown ) {
	    return 0;
	}
    }
    return 0;
}


/*
 * Wake a replication thread which may be sleeping.
 * Send it a LDAP_SIGUSR1.
 */
static void
Ri_wake(
    Ri *ri
) 
{
    if ( ri == NULL ) {
	return;
    }
    ldap_pvt_thread_kill( ri->ri_tid, LDAP_SIGUSR1 );
}



/* 
 * Allocate and initialize an Ri struct.
 */
int
Ri_init(
    Ri	**ri
)
{
    (*ri) = ( Ri * ) calloc( 1, sizeof( Ri ));
    if ( *ri == NULL ) {
	return -1;
    }

    /* Initialize member functions */
    (*ri)->ri_process = Ri_process;
    (*ri)->ri_wake = Ri_wake;

    /* Initialize private data */
    (*ri)->ri_hostname = NULL;
    (*ri)->ri_ldp = NULL;
    (*ri)->ri_bind_dn = NULL;
    (*ri)->ri_password = NULL;
    (*ri)->ri_authcId = NULL;
    (*ri)->ri_srvtab = NULL;
    (*ri)->ri_curr = NULL;

    return 0;
}




/*
 * Return 1 if the hostname and port in re match the hostname and port
 * in ri, otherwise return zero.
 */
static int
ismine(
    Ri	*ri,
    Re	*re
)
{
    Rh	*rh;
    int	i;

    if ( ri == NULL || re == NULL || ri->ri_hostname == NULL ||
	    re->re_replicas == NULL ) {
	return 0;
    }
    rh = re->re_replicas;
    for ( i = 0; rh[ i ].rh_hostname != NULL; i++ ) {
	if ( !strcmp( rh[ i ].rh_hostname, ri->ri_hostname) &&
		rh[ i ].rh_port == ri->ri_port ) {
	    return 1;
	}
    }
    return 0;
}




/*
 * Return 1 if the Re's timestamp/seq combination are greater than the
 * timestamp and seq in the Ri's ri_stel member.  In other words, if we
 * find replication entries in the log which we've already processed,
 * don't process them.  If the re is "old," return 0.
 * No check for NULL pointers is done.
 */
static int
isnew(
    Ri	*ri,
    Re	*re
)
{
    long x;
    int	ret;

    /* Lock the St struct to avoid a race */
    sglob->st->st_lock( sglob->st );
    x = re->re_timestamp - ri->ri_stel->last;
    if ( x > 0 ) {
	/* re timestamp is newer */
	ret = 1;
    } else if ( x < 0 ) {
	ret = 0;
    } else {
	/* timestamps were equal */
	if ( re->re_seq > ri->ri_stel->seq ) {
	    /* re seq is newer */
	    ret = 1;
	} else {
	    ret = 0;
	}
    }
    sglob->st->st_unlock( sglob->st );
    return ret;
}
