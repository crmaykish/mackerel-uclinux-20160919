/* $OpenLDAP: pkg/ldap/include/lutil.h,v 1.37.2.10 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _LUTIL_H
#define _LUTIL_H 1

#include <ldap_cdefs.h>
#include <lber_types.h>

/*
 * Include file for LDAP utility routine
 */

LDAP_BEGIN_DECL

/* n octets encode into ceiling(n/3) * 4 bytes */
/* Avoid floating point math through extra padding */

#define LUTIL_BASE64_ENCODE_LEN(n)	(((n)+2)/3 * 4)
#define LUTIL_BASE64_DECODE_LEN(n)	(((n)+3)/4 * 3)

/* ISC Base64 Routines */
/* base64.c */

LDAP_LUTIL_F( int )
lutil_b64_ntop LDAP_P((
	unsigned char const *,
	size_t,
	char *,
	size_t));

LDAP_LUTIL_F( int )
lutil_b64_pton LDAP_P((
	char const *,
	unsigned char *,
	size_t));

/* detach.c */
LDAP_LUTIL_F( void )
lutil_detach LDAP_P((
	int debug,
	int do_close));

/* entropy.c */
LDAP_LUTIL_F( int )
lutil_entropy LDAP_P((
	unsigned char *buf,
	ber_len_t nbytes ));

/* passfile.c */
struct berval; /* avoid pulling in lber.h */

LDAP_LUTIL_F( int )
lutil_get_filed_password LDAP_P((
	const char *filename,
	struct berval * ));

/* passwd.c */
LDAP_LUTIL_F( int )
lutil_authpasswd LDAP_P((
	const struct berval *passwd,	/* stored password */
	const struct berval *cred,	/* user supplied value */
	const char **methods ));

LDAP_LUTIL_F( int )
lutil_authpasswd_hash LDAP_P((
	const struct berval *cred,
	struct berval **passwd,	/* password to store */
	struct berval **salt,	/* salt to store */
	const char *method ));

#if defined( SLAPD_SPASSWD ) && defined( HAVE_CYRUS_SASL )
	/* cheat to avoid pulling in <sasl.h> */
LDAP_LUTIL_V( struct sasl_conn * ) lutil_passwd_sasl_conn;
#endif

LDAP_LUTIL_F( int )
lutil_passwd LDAP_P((
	const struct berval *passwd,	/* stored password */
	const struct berval *cred,	/* user supplied value */
	const char **methods ));

LDAP_LUTIL_F( struct berval * )
lutil_passwd_generate LDAP_P(( ber_len_t ));

LDAP_LUTIL_F( struct berval * )
lutil_passwd_hash LDAP_P((
	const struct berval *passwd,
	const char *method ));

LDAP_LUTIL_F( int )
lutil_passwd_scheme LDAP_P((
	const char *scheme ));

LDAP_LUTIL_F( int )
lutil_salt_format LDAP_P((
	const char *format ));

/* utils.c */
LDAP_LUTIL_F( char* )
lutil_progname LDAP_P((
	const char* name,
	int argc,
	char *argv[] ));

LDAP_LUTIL_F( char* )
lutil_strcopy LDAP_P(( char *dst, const char *src ));

LDAP_LUTIL_F( char* )
lutil_strncopy LDAP_P(( char *dst, const char *src, size_t n ));

struct tm;

/* use this macro to statically allocate buffer for lutil_gentime */
#define LDAP_LUTIL_GENTIME_BUFSIZE	22
LDAP_LUTIL_F( size_t )
lutil_gentime LDAP_P(( char *s, size_t max, const struct tm *tm ));

#ifndef HAVE_MKSTEMP
LDAP_LUTIL_F( int )
mkstemp LDAP_P (( char * template ));
#endif

/* sockpair.c */
LDAP_LUTIL_F( int )
lutil_pair( ber_socket_t sd[2] );

/* uuid.c */
/* use this macro to allocate buffer for lutil_uuidstr */
#define LDAP_LUTIL_UUIDSTR_BUFSIZE	40
LDAP_LUTIL_F( size_t )
lutil_uuidstr( char *buf, size_t len );

/* csn.c */
/* use this macro to allocate buffer for lutil_csnstr */
#define LDAP_LUTIL_CSNSTR_BUFSIZE	64
LDAP_LUTIL_F( size_t )
lutil_csnstr( char *buf, size_t len, unsigned int replica, unsigned int mod );

/*
 * Sometimes not all declarations in a header file are needed.
 * An indicator to this is whether or not the symbol's type has
 * been defined. Thus, we don't need to include a symbol if
 * its type has not been defined through another header file.
 */

#ifdef HAVE_NT_SERVICE_MANAGER
LDAP_LUTIL_V (int) is_NT_Service;

#ifdef _LDAP_PVT_THREAD_H
LDAP_LUTIL_V (ldap_pvt_thread_cond_t) started_event;
#endif /* _LDAP_PVT_THREAD_H */

/* macros are different between Windows and Mingw */
#if defined(_WINSVC_H) || defined(_WINSVC_)
LDAP_LUTIL_V (SERVICE_STATUS) lutil_ServiceStatus;
LDAP_LUTIL_V (SERVICE_STATUS_HANDLE) hlutil_ServiceStatus;
#endif /* _WINSVC_H */

LDAP_LUTIL_F (void)
lutil_CommenceStartupProcessing( char *serverName, void (*stopper)(int)) ;

LDAP_LUTIL_F (void)
lutil_ReportShutdownComplete( void );

LDAP_LUTIL_F (void *)
lutil_getRegParam( char *svc, char *value );

LDAP_LUTIL_F (int)
lutil_srv_install( char* service, char * displayName, char* filename,
		 int auto_start );
LDAP_LUTIL_F (int)
lutil_srv_remove ( char* service, char* filename );

#endif /* HAVE_NT_SERVICE_MANAGER */

#ifdef HAVE_NT_EVENT_LOG
LDAP_LUTIL_F (void)
lutil_LogStartedEvent( char *svc, int slap_debug, char *configfile, char *urls );

LDAP_LUTIL_F (void)
lutil_LogStoppedEvent( char *svc );
#endif

#ifdef HAVE_EBCDIC
/* Generally this has only been used to put '\n' to stdout. We need to
 * make sure it is output in EBCDIC.
 */
#undef putchar
#undef putc
#define putchar(c)     putc((c), stdout)
#define putc(c,fp)     do { char x=(c); __atoe_l(&x,1); putc(x,fp); } while(0)
#endif

LDAP_END_DECL

#endif /* _LUTIL_H */
