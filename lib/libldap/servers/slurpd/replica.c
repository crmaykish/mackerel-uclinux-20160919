/* $OpenLDAP: pkg/ldap/servers/slurpd/replica.c,v 1.16.2.4 2003/03/03 17:10:11 kurt Exp $ */
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
 * replica.c - code to start up replica threads.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/stdlib.h>

#include "slurp.h"
#include "globals.h"

/*
 * Just invoke the Ri's process() member function, and log the start and
 * finish.
 */
static void *
replicate(
    void	*ri_arg
)
{
    Ri		*ri = (Ri *) ri_arg;

#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ARGS, "replicate: "
		"begin replication thread for %s:%d\n",
	    ((Ri *)ri)->ri_hostname, ((Ri *)ri)->ri_port, 0 );
#else
    Debug( LDAP_DEBUG_ARGS, "begin replication thread for %s:%d\n",
	    ((Ri *)ri)->ri_hostname, ((Ri *)ri)->ri_port, 0 );
#endif

    ri->ri_process( ri );

#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ARGS, "replicate: "
		"begin replication thread for %s:%d\n",
	    ri->ri_hostname, ri->ri_port, 0 );
#else
    Debug( LDAP_DEBUG_ARGS, "end replication thread for %s:%d\n",
	    ri->ri_hostname, ri->ri_port, 0 );
#endif
    return NULL;
}



/*
 * Start a detached thread for the given replica.
 */
int
start_replica_thread(
    Ri	*ri
)
{
    /* POSIX_THREADS or compatible */
    if ( ldap_pvt_thread_create( &(ri->ri_tid), 0, replicate,
	    (void *) ri ) != 0 ) {
#ifdef NEW_LOGGING
	LDAP_LOG ( SLURPD, ERR, "start_replica_thread: "
		"replica %s:%d ldap_pvt_thread_create failed\n",
	    ri->ri_hostname, ri->ri_port, 0 );
#else
	Debug( LDAP_DEBUG_ANY, "replica \"%s:%d\" ldap_pvt_thread_create failed\n",
		ri->ri_hostname, ri->ri_port, 0 );
#endif
	return -1;
    }

    return 0;
}
