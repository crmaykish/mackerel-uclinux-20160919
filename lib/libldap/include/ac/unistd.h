/* Generic unistd.h */
/* $OpenLDAP: pkg/ldap/include/ac/unistd.h,v 1.31.2.1 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_UNISTD_H
#define _AC_UNISTD_H

#if HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#	include <unistd.h>
#endif

#if HAVE_PROCESS_H
#	include <process.h>
#endif

/* note: callers of crypt(3) should include <ac/crypt.h> */

#if defined(HAVE_GETPASSPHRASE)
LDAP_LIBC_F(char*)(getpassphrase)();

#elif defined(HAVE_GETPASS)
#define getpassphrase(p) getpass(p)
LDAP_LIBC_F(char*)(getpass)();

#else
#define NEED_GETPASSPHRASE 1
#define getpassphrase(p) lutil_getpass(p)
LDAP_LUTIL_F(char*)(lutil_getpass) LDAP_P((const char *getpass));
#endif

/* getopt() defines may be in separate include file */
#if HAVE_GETOPT_H
#	include <getopt.h>

#elif !defined(HAVE_GETOPT)
	/* no getopt, assume we need getopt-compat.h */
#	include <getopt-compat.h>

#else
	/* assume we need to declare these externs */
	LDAP_LIBC_V (char *) optarg;
	LDAP_LIBC_V (int) optind, opterr, optopt;
#endif

/* use lutil file locking */
#define ldap_lockf(x)	lutil_lockf(x)
#define ldap_unlockf(x)	lutil_unlockf(x)
#include <lutil_lockf.h>

/*
 * Windows: although sleep() will be resolved by both MSVC and Mingw GCC
 * linkers, the function is not declared in header files. This is
 * because Windows' version of the function is called _sleep(), and it
 * is declared in stdlib.h
 */

#ifdef _WIN32
#define sleep _sleep
#endif

#endif /* _AC_UNISTD_H */
