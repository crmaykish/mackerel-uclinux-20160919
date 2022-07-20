/* $OpenLDAP: pkg/ldap/libraries/libldap/print.c,v 1.10.2.2 2003/03/03 17:10:05 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/stdarg.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

/*
 * ldap log 
 */

static int ldap_log_check( LDAP *ld, int loglvl )
{
	int errlvl;

	if(ld == NULL) {
		errlvl = ldap_debug;
	} else {
		errlvl = ld->ld_debug;
	}

	return errlvl & loglvl ? 1 : 0;
}

int ldap_log_printf( LDAP *ld, int loglvl, const char *fmt, ... )
{
	char buf[ 1024 ];
	va_list ap;

	if ( !ldap_log_check( ld, loglvl )) {
		return 0;
	}

	va_start( ap, fmt );

	buf[sizeof(buf) - 1] = '\0';
	vsnprintf( buf, sizeof(buf)-1, fmt, ap );

	va_end(ap);

	(*ber_pvt_log_print)( buf );
	return 1;
}
