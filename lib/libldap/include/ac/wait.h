/* Generic wait.h */
/* $OpenLDAP: pkg/ldap/include/ac/wait.h,v 1.10.2.1 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_WAIT_H
#define _AC_WAIT_H

#include <sys/types.h>

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#define LDAP_HI(s)	(((s) >> 8) & 0377)
#define LDAP_LO(s)	((s) & 0377)

/* These should work on non-POSIX UNIX platforms,
	all bets on off on non-POSIX non-UNIX platforms... */
#ifndef WIFEXITED
# define WIFEXITED(s)	(LDAP_LO(s) == 0)
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(s) LDAP_HI(s)
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(s) (LDAP_LO(s) > 0 && LDAP_HI(s) == 0)
#endif
#ifndef WTERMSIG
# define WTERMSIG(s)	(LDAP_LO(s) & 0177)
#endif
#ifndef WIFSTOPPED
# define WIFSTOPPED(s)	(LDAP_LO(s) == 0177 && LDAP_HI(s) != 0)
#endif
#ifndef WSTOPSIG
# define WSTOPSIG(s)	LDAP_HI(s)
#endif

#ifdef WCONTINUED
# define WAIT_FLAGS ( WNOHANG | WUNTRACED | WCONTINUED )
#else
# define WAIT_FLAGS ( WNOHANG | WUNTRACED )
#endif

#endif /* _AC_WAIT_H */
