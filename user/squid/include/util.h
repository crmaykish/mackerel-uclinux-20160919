/*
 * $Id: util.h,v 1.62 2001/10/17 01:36:07 hno Exp $
 *
 * AUTHOR: Harvest Derived
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

#ifndef SQUID_UTIL_H
#define SQUID_UTIL_H

#include "config.h"
#include <stdio.h>
#include <time.h>
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if !defined(SQUIDHOSTNAMELEN)
#include <sys/param.h>
#ifndef _SQUID_NETDB_H_		/* need protection on NEXTSTEP */
#define _SQUID_NETDB_H_
#include <netdb.h>
#endif
#if !defined(MAXHOSTNAMELEN) || (MAXHOSTNAMELEN < 128)
#define SQUIDHOSTNAMELEN 128
#else
#define SQUIDHOSTNAMELEN MAXHOSTNAMELEN
#endif
#endif

#if defined(_SQUID_FREEBSD_)
#define _etext etext
#endif

extern const char *getfullhostname(void);
extern const char *mkhttpdlogtime(const time_t *);
extern const char *mkrfc1123(time_t);
extern char *uudecode(const char *);
extern char *xstrdup(const char *);
extern char *xstrndup(const char *, size_t);
extern const char *xstrerror(void);
extern const char *xbstrerror(int);
extern int tvSubMsec(struct timeval, struct timeval);
extern int tvSubUsec(struct timeval, struct timeval);
extern double tvSubDsec(struct timeval, struct timeval);
extern char *xstrncpy(char *, const char *, size_t);
extern size_t xcountws(const char *str);
extern time_t parse_rfc1123(const char *str);
extern void *xcalloc(size_t, size_t);
extern void *xmalloc(size_t);
extern void *xrealloc(void *, size_t);
extern void Tolower(char *);
extern void xfree(void *);
extern void xxfree(const void *);

/* rfc1738.c */
extern char *rfc1738_escape(const char *);
extern char *rfc1738_escape_unescaped(const char *);
extern char *rfc1738_escape_part(const char *);
extern void rfc1738_unescape(char *);

/* html.c */
extern char *html_quote(const char *);

#if XMALLOC_STATISTICS
extern void malloc_statistics(void (*)(int, int, int, void *), void *);
#endif

#if XMALLOC_TRACE
#define xmalloc(size) (xmalloc_func="xmalloc",xmalloc_line=__LINE__,xmalloc_file=__FILE__,xmalloc(size))
#define xfree(ptr) (xmalloc_func="xfree",xmalloc_line=__LINE__,xmalloc_file=__FILE__,xfree(ptr))
#define xxfree(ptr) (xmalloc_func="xxfree",xmalloc_line=__LINE__,xmalloc_file=__FILE__,xxfree(ptr))
#define xrealloc(ptr,size) (xmalloc_func="xrealloc",xmalloc_line=__LINE__,xmalloc_file=__FILE__,xrealloc(ptr,size))
#define xcalloc(n,size) (xmalloc_func="xcalloc",xmalloc_line=__LINE__,xmalloc_file=__FILE__,xcalloc(n,size))
#define xstrdup(ptr) (xmalloc_func="xstrdup",xmalloc_line=__LINE__,xmalloc_file=__FILE__,xstrdup(ptr))
extern int xmalloc_line;
extern char *xmalloc_file;
extern char *xmalloc_func;
extern int xmalloc_trace;
extern size_t xmalloc_total;
extern void xmalloc_find_leaks(void);
#endif

typedef struct in_addr SIA;
extern int safe_inet_addr(const char *, SIA *);
extern time_t parse_iso3307_time(const char *buf);
extern char *base64_decode(const char *coded);
extern const char *base64_encode(const char *decoded);
extern const char *base64_encode_bin(const char *data, int len);

extern double xpercent(double part, double whole);
extern int xpercentInt(double part, double whole);
extern double xdiv(double nom, double denom);

extern const char *xitoa(int num);

#if !HAVE_DRAND48
double drand48(void);
#endif

/*
 * Returns the amount of known allocated memory
 */
int statMemoryAccounted(void);

#ifndef HAVE_STRNSTR
extern char *strnstr(const char *haystack, const char *needle, size_t haystacklen);
#endif

#ifndef HAVE_STRCASESTR
extern char *strcasestr(const char *haystack, const char *needle);
#endif

#endif /* SQUID_UTIL_H */
