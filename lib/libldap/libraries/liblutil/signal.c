/* $OpenLDAP: pkg/ldap/libraries/liblutil/signal.c,v 1.5.2.1 2003/03/03 17:10:06 kurt Exp $ */
/*
 * Copyright 2000-2003 The OpenLDAP Foundation
 * COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
 * of this package for details.
 */

#include "portable.h"

#ifdef HAVE_SIGACTION
#include <ac/string.h>
#include <ac/signal.h>

lutil_sig_t
lutil_sigaction(int sig, lutil_sig_t func)
{
	struct sigaction action, oaction;

	memset( &action, '\0', sizeof(action) );

	action.sa_handler = func;
	sigemptyset( &action.sa_mask );
#ifdef SA_RESTART
	action.sa_flags |= SA_RESTART;
#endif
	
	if( sigaction( sig, &action, &oaction ) != 0 ) {
		return NULL;
	}

	return oaction.sa_handler;
}
#endif
