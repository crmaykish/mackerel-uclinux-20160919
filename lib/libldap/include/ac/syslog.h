/* Generic syslog.h */
/* $OpenLDAP: pkg/ldap/include/ac/syslog.h,v 1.11.2.1 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_SYSLOG_H_
#define _AC_SYSLOG_H_

#if defined( HAVE_SYSLOG_H )
#include <syslog.h>
#elif defined ( HAVE_SYS_SYSLOG_H )
#include <sys/syslog.h>
#endif

#if defined( LOG_NDELAY ) && defined( LOG_NOWAIT )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NDELAY | LOG_NOWAIT )
#elif defined( LOG_NDELAY )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NDELAY )
#elif defined( LOG_NOWAIT )
#	define OPENLOG_OPTIONS ( LOG_PID | LOG_NOWAIT )
#elif defined( LOG_PID )
#	define OPENLOG_OPTIONS ( LOG_PID )
#else
#   define OPENLOG_OPTIONS ( 0 )
#endif

#endif /* _AC_SYSLOG_H_ */
