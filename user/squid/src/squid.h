
/*
 * $Id: squid.h,v 1.216.2.8 2005/03/26 02:50:53 hno Exp $
 *
 * AUTHOR: Duane Wessels
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#ifndef SQUID_H
#define SQUID_H

#include "config.h"

/*
 * experimental defines for ICAP
 */
#ifdef HS_FEAT_ICAP
#define ICAP_PREVIEW 1
#define SUPPORT_ICAP_204 0
#endif

/*
 * On some systems, FD_SETSIZE is set to something lower than the
 * actual number of files which can be opened.  IRIX is one case,
 * NetBSD is another.  So here we increase FD_SETSIZE to our
 * configure-discovered maximum *before* any system includes.
 */
#define CHANGE_FD_SETSIZE 1

/*
 * Cannot increase FD_SETSIZE on Linux, but we can increase __FD_SETSIZE
 * with glibc 2.2 (or later? remains to be seen). We do this by including
 * bits/types.h which defines __FD_SETSIZE first, then we redefine
 * __FD_SETSIZE. Ofcourse a user program may NEVER include bits/whatever.h
 * directly, so this is a dirty hack!
 */
#if defined(_SQUID_LINUX_)
#undef CHANGE_FD_SETSIZE
#define CHANGE_FD_SETSIZE 0
#include <features.h>
#if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)
#if SQUID_MAXFD > DEFAULT_FD_SETSIZE
#include <bits/types.h>
#undef __FD_SETSIZE
#define __FD_SETSIZE SQUID_MAXFD
#endif
#endif
#endif

/*
 * Cannot increase FD_SETSIZE on FreeBSD before 2.2.0, causes select(2)
 * to return EINVAL.
 * --Marian Durkovic <marian@svf.stuba.sk>
 * --Peter Wemm <peter@spinner.DIALix.COM>
 */
#if defined(_SQUID_FREEBSD_)
#include <osreldate.h>
#if __FreeBSD_version < 220000
#undef CHANGE_FD_SETSIZE
#define CHANGE_FD_SETSIZE 0
#endif
#endif

/*
 * Trying to redefine CHANGE_FD_SETSIZE causes a slew of warnings
 * on Mac OS X Server.
 */
#if defined(_SQUID_APPLE_)
#undef CHANGE_FD_SETSIZE
#define CHANGE_FD_SETSIZE 0
#endif

/* Increase FD_SETSIZE if SQUID_MAXFD is bigger */
#if CHANGE_FD_SETSIZE && SQUID_MAXFD > DEFAULT_FD_SETSIZE
#define FD_SETSIZE SQUID_MAXFD
#endif

#if PURIFY
#define assert(EX) ((void)0)
#elif defined(NODEBUG)
#define assert(EX) ((void)0)
#elif STDC_HEADERS
#define assert(EX)  ((EX)?((void)0):xassert( # EX , __FILE__, __LINE__))
#else
#define assert(EX)  ((EX)?((void)0):xassert("EX", __FILE__, __LINE__))
#endif


/* 32 bit integer compatability */
#include "squid_types.h"
#define num32 int32_t
#define u_num32 u_int32_t

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif
#if HAVE_GNUMALLOC_H
#include <gnumalloc.h>
#elif HAVE_MALLOC_H && !defined(_SQUID_FREEBSD_) && !defined(_SQUID_NEXT_)
#include <malloc.h>
#endif
#if HAVE_MEMORY_H
#include <memory.h>
#endif
#if HAVE_NETDB_H && !defined(_SQUID_NETDB_H_)	/* protect NEXTSTEP */
#define _SQUID_NETDB_H_
#ifdef _SQUID_NEXT_
#include <netinet/in_systm.h>
#endif
#include <netdb.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>	/* needs sys/time.h above it */
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_LIBC_H
#include <libc.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_BSTRING_H
#include <bstring.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#if defined(_SQUID_CYGWIN_)
#include <io.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* HAVE_DIRENT_H */
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#if HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H */

#if defined(__QNX__)
#include <unix.h>
#endif

#if HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

/*
 * We require poll.h before using poll().  If the symbols used
 * by poll() are defined elsewhere, we will need to make this
 * a more sophisticated test.
 *  -- Oskar Pearson <oskar@is.co.za>
 *  -- Stewart Forster <slf@connect.com.au>
 */
#if HAVE_POLL
#if HAVE_POLL_H
#include <poll.h>
#else /* HAVE_POLL_H */
#undef HAVE_POLL
#endif /* HAVE_POLL_H */
#endif /* HAVE_POLL */

#if defined(HAVE_STDARG_H)
#include <stdarg.h>
#define HAVE_STDARGS		/* let's hope that works everywhere (mj) */
#define VA_LOCAL_DECL va_list ap;
#define VA_START(f) va_start(ap, f)
#define VA_SHIFT(v,t) ;		/* no-op for ANSI */
#define VA_END va_end(ap)
#else
#if defined(HAVE_VARARGS_H)
#include <varargs.h>
#undef HAVE_STDARGS
#define VA_LOCAL_DECL va_list ap;
#define VA_START(f) va_start(ap)	/* f is ignored! */
#define VA_SHIFT(v,t) v = va_arg(ap,t)
#define VA_END va_end(ap)
#else
#error XX **NO VARARGS ** XX
#endif
#endif

/* Make sure syslog goes after stdarg/varargs */
#ifdef HAVE_SYSLOG_H
#ifdef _SQUID_AIX_
#define _XOPEN_EXTENDED_SOURCE
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#include <syslog.h>
#endif

#if HAVE_MATH_H
#include <math.h>
#endif

#if !defined(MAXHOSTNAMELEN) || (MAXHOSTNAMELEN < 128)
#define SQUIDHOSTNAMELEN 128
#else
#define SQUIDHOSTNAMELEN MAXHOSTNAMELEN
#endif

#define SQUID_MAXPATHLEN 256
#ifndef MAXPATHLEN
#define MAXPATHLEN SQUID_MAXPATHLEN
#endif

#if !HAVE_GETRUSAGE
#if defined(_SQUID_HPUX_)
#define HAVE_GETRUSAGE 1
#define getrusage(a, b)  syscall(SYS_GETRUSAGE, a, b)
#endif
#endif

#if !HAVE_STRUCT_RUSAGE
/*
 * If we don't have getrusage() then we create a fake structure
 * with only the fields Squid cares about.  This just makes the
 * source code cleaner, so we don't need lots of #ifdefs in other
 * places
 */
struct rusage {
    struct timeval ru_stime;
    struct timeval ru_utime;
    int ru_maxrss;
    int ru_majflt;
};

#endif

#if !defined(HAVE_GETPAGESIZE) && defined(_SQUID_HPUX_)
#define HAVE_GETPAGESIZE
#define getpagesize( )   sysconf(_SC_PAGE_SIZE)
#endif

#ifndef BUFSIZ
#define BUFSIZ  4096		/* make reasonable guess */
#endif


#ifndef SA_RESTART
#define SA_RESTART 0
#endif
#ifndef SA_NODEFER
#define SA_NODEFER 0
#endif
#ifndef SA_RESETHAND
#define SA_RESETHAND 0
#endif
#if SA_RESETHAND == 0 && defined(SA_ONESHOT)
#undef SA_RESETHAND
#define SA_RESETHAND SA_ONESHOT
#endif

#if PURIFY
#define LOCAL_ARRAY(type,name,size) \
        static type *local_##name=NULL; \
        type *name = local_##name ? local_##name : \
                ( local_##name = (type *)xcalloc(size, sizeof(type)) )
#else
#define LOCAL_ARRAY(type,name,size) static type name[size]
#endif

#if CBDATA_DEBUG
#define cbdataAlloc(a,b)	cbdataAllocDbg(a,b,__FILE__,__LINE__)
#define cbdataLock(a)		cbdataLockDbg(a,__FILE__,__LINE__)
#define cbdataUnlock(a)		cbdataUnlockDbg(a,__FILE__,__LINE__)
#endif

#if USE_LEAKFINDER
#define leakAdd(p) leakAddFL(p,__FILE__,__LINE__)
#define leakTouch(p) leakTouchFL(p,__FILE__,__LINE__)
#define leakFree(p) leakFreeFL(p,__FILE__,__LINE__)
#else
#define leakAdd(p) p
#define leakTouch(p) p
#define leakFree(p) p
#endif

#if defined(_SQUID_NEXT_) && !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & (_S_IFMT)) == (_S_IFDIR))
#endif

#ifdef USE_GNUREGEX
#include "GNUregex.h"
#elif HAVE_REGEX_H
#include <regex.h>
#endif

#include "md5.h"

#if USE_SSL
#include "ssl_support.h"
#endif

#include "Stack.h"

/* Needed for poll() on Linux at least */
#if HAVE_POLL
#ifndef POLLRDNORM
#define POLLRDNORM POLLIN
#endif
#ifndef POLLWRNORM
#define POLLWRNORM POLLOUT
#endif
#endif

#ifdef SQUID_SNMP
#include "cache_snmp.h"
#endif

#include "hash.h"
#include "rfc1035.h"

#include "defines.h"
#include "enums.h"
#include "typedefs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"

#include "util.h"

#if !HAVE_TEMPNAM
#include "tempnam.h"
#endif

#if !HAVE_SNPRINTF
#include "snprintf.h"
#endif

#if !HAVE_INITGROUPS
#include "initgroups.h"
#endif

#define XMIN(x,y) ((x)<(y)? (x) : (y))
#define XMAX(a,b) ((a)>(b)? (a) : (b))

/*
 * Squid source files should not call these functions directly.
 * Use xmalloc, xfree, xcalloc, snprintf, and xstrdup instead.
 * Also use xmemcpy, xisspace, ...
 */
#ifndef malloc
#define malloc +
#endif
#ifndef free
#define free +
#endif
#ifndef calloc
#define calloc +
#endif
#ifndef sprintf
#define sprintf +
#endif
#ifndef strdup
#define strdup +
#endif

/*
 * Hey dummy, don't be tempted to move this to lib/config.h.in
 * again.  O_NONBLOCK will not be defined there because you didn't
 * #include <fcntl.h> yet.
 */
#if defined(_SQUID_SUNOS_)
/*
 * We assume O_NONBLOCK is broken, or does not exist, on SunOS.
 */
#define SQUID_NONBLOCK O_NDELAY
#elif defined(O_NONBLOCK)
/*
 * We used to assume O_NONBLOCK was broken on Solaris, but evidence
 * now indicates that its fine on Solaris 8, and in fact required for
 * properly detecting EOF on FIFOs.  So now we assume that if 
 * its defined, it works correctly on all operating systems.
 */
#define SQUID_NONBLOCK O_NONBLOCK
/*
 * O_NDELAY is our fallback.
 */
#else
#define SQUID_NONBLOCK O_NDELAY
#endif

/*
 * I'm sick of having to keep doing this ..
 */
#define INDEXSD(i)   (&Config.cacheSwap.swapDirs[(i)])

#define FD_READ_METHOD(fd, buf, len) (*fd_table[fd].read_method)(fd, buf, len)
#define FD_WRITE_METHOD(fd, buf, len) (*fd_table[fd].write_method)(fd, buf, len)

/*
 * Trap attempts to build large file cache support without support for
 * large objects
 */
#if LARGE_CACHE_FILES && SIZEOF_SQUID_OFF_T <= 4
#error Your platform does not support large integers. Can not build with --enable-large-cache-files
#endif

#endif /* SQUID_H */
