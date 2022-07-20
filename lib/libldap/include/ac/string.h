/* Generic string.h */
/* $OpenLDAP: pkg/ldap/include/ac/string.h,v 1.36.2.2 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_STRING_H
#define _AC_STRING_H

#ifdef STDC_HEADERS
#	include <string.h>

#else
#	ifdef HAVE_STRING_H
#		include <string.h>
#	endif
#	if defined(HAVE_STRINGS_H) && (!defined(HAVE_STRING_H) || defined(BOTH_STRINGS_H))
#		include <strings.h>
#	endif

#	ifdef HAVE_MEMORY_H
#		include <memory.h>
#	endif

#	ifndef HAVE_STRRCHR
#		undef strchr
#		define strchr index
#		undef strrchr
#		define strrchr rindex
#	endif

#	ifndef HAVE_MEMCPY
#		undef memcpy
#		define memcpy(d, s, n)		((void) bcopy ((s), (d), (n)))
#		undef memmove
#		define memmove(d, s, n)		((void) bcopy ((s), (d), (n)))
#	endif
#endif

/* use ldap_pvt_strtok instead of strtok or strtok_r! */
LDAP_F(char *) ldap_pvt_strtok LDAP_P(( char *str,
	const char *delim, char **pos ));

#ifndef HAVE_STRDUP
	/* strdup() is missing, declare our own version */
#	undef strdup
#	define strdup(s) ber_strdup(s)
#elif !defined(_WIN32)
	/* some systems fail to declare strdup */
	/* Windows does not require this declaration */
	LDAP_LIBC_F(char *) (strdup)();
#endif

/*
 * some systems fail to declare strcasecmp() and strncasecmp()
 * we need them declared so we can obtain pointers to them
 */

/* we don't want these declared for Windows or Mingw */
#ifndef _WIN32
int (strcasecmp)();
int (strncasecmp)();
#endif

#ifndef SAFEMEMCPY
#	if defined( HAVE_MEMMOVE )
#		define SAFEMEMCPY( d, s, n ) 	memmove((d), (s), (n))
#	elif defined( HAVE_BCOPY )
#		define SAFEMEMCPY( d, s, n ) 	bcopy((s), (d), (n))
#	else
		/* nothing left but memcpy() */
#		define SAFEMEMCPY( d, s, n )	memcpy((d), (s), (n))
#	endif
#endif

#define AC_MEMCPY( d, s, n ) (SAFEMEMCPY((d),(s),(n)))
#define AC_FMEMCPY( d, s, n ) do { \
		if((n) == 1) *((char*)(d)) = *((char*)(s)); \
		else AC_MEMCPY( (d), (s), (n) ); \
	} while(0)

#endif /* _AC_STRING_H */
