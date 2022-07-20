/* Generic errno.h */
/* $OpenLDAP: pkg/ldap/include/ac/errno.h,v 1.24.2.1 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_ERRNO_H
#define _AC_ERRNO_H

#if defined( HAVE_ERRNO_H )
# include <errno.h>
#elif defined( HAVE_SYS_ERRNO_H )
# include <sys/errno.h>
#endif

#ifndef HAVE_SYS_ERRLIST
	/* no sys_errlist */
#	define		sys_nerr	0
#	define		sys_errlist	((char **)0)
#elif DECL_SYS_ERRLIST
	/* have sys_errlist but need declaration */
	LDAP_LIBC_V(int)      sys_nerr;
	LDAP_LIBC_V(char)    *sys_errlist[];
#endif

#undef _AC_ERRNO_UNKNOWN
#define _AC_ERRNO_UNKNOWN "unknown error"

#ifdef HAVE_SYS_ERRLIST
	/* this is thread safe */
#	define	STRERROR(e) ( (e) > -1 && (e) < sys_nerr \
			? sys_errlist[(e)] : _AC_ERRNO_UNKNOWN )

#elif defined( HAVE_STRERROR )
	/* this may not be thread safe */
	/* and, yes, some implementations of strerror may return NULL */
#	define	STRERROR(e) ( strerror(e) \
		? strerror(e) : _AC_ERRNO_UNKNOWN )

#else
	/* this is thread safe */
#	define	STRERROR(e) ( _AC_ERRNO_UNKNOWN )
#endif

#endif /* _AC_ERRNO_H */
