/* $OpenLDAP: pkg/ldap/include/ldap_pvt.h,v 1.58.2.7 2003/03/05 23:48:31 kurt Exp $ */
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
/*
 * ldap-pvt.h - Header for ldap_pvt_ functions. These are meant to be used
 * 		by the OpenLDAP distribution only.
 */

#ifndef _LDAP_PVT_H
#define _LDAP_PVT_H 1

#include <lber.h>				/* get ber_slen_t */

LDAP_BEGIN_DECL

#define LDAP_PROTO_TCP 1 /* ldap://  */
#define LDAP_PROTO_UDP 2 /* reserved */
#define LDAP_PROTO_IPC 3 /* ldapi:// */

LDAP_F ( int )
ldap_pvt_url_scheme2proto LDAP_P((
	const char * ));
LDAP_F ( int )
ldap_pvt_url_scheme2tls LDAP_P((
	const char * ));

struct ldap_url_desc; /* avoid pulling in <ldap.h> */

LDAP_F( int )
ldap_url_parse_ext LDAP_P((
	LDAP_CONST char *url,
	struct ldap_url_desc **ludpp ));

LDAP_F( char * )
ldap_pvt_ctime LDAP_P((
	const time_t *tp,
	char *buf ));

LDAP_F( char *) ldap_pvt_get_fqdn LDAP_P(( char * ));

struct hostent;	/* avoid pulling in <netdb.h> */

LDAP_F( int )
ldap_pvt_gethostbyname_a LDAP_P((
	const char *name,
	struct hostent *resbuf,
	char **buf,
	struct hostent **result,
	int *herrno_ptr ));

LDAP_F( int )
ldap_pvt_gethostbyaddr_a LDAP_P((
	const char *addr,
	int len,
	int type,
	struct hostent *resbuf,
	char **buf,
	struct hostent **result,
	int *herrno_ptr ));

struct sockaddr;

LDAP_F( int )
ldap_pvt_get_hname LDAP_P((
	const struct sockaddr * sa,
	int salen,
	char *name,
	int namelen,
	char **herr ));


/* charray.c */

LDAP_F( int )
ldap_charray_add LDAP_P((
    char	***a,
    char	*s ));

LDAP_F( int )
ldap_charray_merge LDAP_P((
    char	***a,
    char	**s ));

LDAP_F( void )
ldap_charray_free LDAP_P(( char **a ));

LDAP_F( int )
ldap_charray_inlist LDAP_P((
    char	**a,
    char	*s ));

LDAP_F( char ** )
ldap_charray_dup LDAP_P(( char **a ));

LDAP_F( char ** )
ldap_str2charray LDAP_P((
	const char *str,
	const char *brkstr ));

LDAP_F( char * )
ldap_charray2str LDAP_P((
	char **array, const char* sep ));

/* url.c */
LDAP_F (void) ldap_pvt_hex_unescape LDAP_P(( char *s ));

/*
 * these macros assume 'x' is an ASCII x
 * and assume the "C" locale
 */
#define LDAP_ASCII(c)		(!((c) & 0x80))
#define LDAP_SPACE(c)		((c) == ' ' || (c) == '\t' || (c) == '\n')
#define LDAP_DIGIT(c)		((c) >= '0' && (c) <= '9')
#define LDAP_LOWER(c)		((c) >= 'a' && (c) <= 'z')
#define LDAP_UPPER(c)		((c) >= 'A' && (c) <= 'Z')
#define LDAP_ALPHA(c)		(LDAP_LOWER(c) || LDAP_UPPER(c))
#define LDAP_ALNUM(c)		(LDAP_ALPHA(c) || LDAP_DIGIT(c))

#define LDAP_LDH(c)			(LDAP_ALNUM(c) || (c) == '-')

#define LDAP_HEXLOWER(c)	((c) >= 'a' && (c) <= 'f')
#define LDAP_HEXUPPER(c)	((c) >= 'A' && (c) <= 'F')
#define LDAP_HEX(c)			(LDAP_DIGIT(c) || \
								LDAP_HEXLOWER(c) || LDAP_HEXUPPER(c))

/* controls.c */
struct ldapcontrol;
LDAP_F (struct ldapcontrol *) ldap_control_dup LDAP_P((
	const struct ldapcontrol *ctrl ));

LDAP_F (struct ldapcontrol **) ldap_controls_dup LDAP_P((
	struct ldapcontrol *const *ctrls ));


#ifdef HAVE_CYRUS_SASL
/* cyrus.c */
struct sasl_security_properties; /* avoid pulling in <sasl.h> */
LDAP_F (int) ldap_pvt_sasl_secprops LDAP_P((
	const char *in,
	struct sasl_security_properties *secprops ));

LDAP_F (void *) ldap_pvt_sasl_mutex_new LDAP_P((void));
LDAP_F (int) ldap_pvt_sasl_mutex_lock LDAP_P((void *mutex));
LDAP_F (int) ldap_pvt_sasl_mutex_unlock LDAP_P((void *mutex));
LDAP_F (void) ldap_pvt_sasl_mutex_dispose LDAP_P((void *mutex));

struct sockbuf; /* avoid pulling in <lber.h> */
LDAP_F (int) ldap_pvt_sasl_install LDAP_P(( struct sockbuf *, void * ));
#endif /* HAVE_CYRUS_SASL */

#define LDAP_PVT_SASL_LOCAL_SSF	71	/* SSF for Unix Domain Sockets */

struct ldap;

LDAP_F (int) ldap_open_internal_connection LDAP_P((
	struct ldap **ldp, ber_socket_t *fdp ));

/* search.c */
LDAP_F( int ) ldap_pvt_put_filter LDAP_P((
	BerElement *ber,
	const char *str ));

LDAP_F( char * )
ldap_pvt_find_wildcard LDAP_P((	const char *s ));

LDAP_F( ber_slen_t )
ldap_pvt_filter_value_unescape LDAP_P(( char *filter ));

/* string.c */
LDAP_F( char * )
ldap_pvt_str2upper LDAP_P(( char *str ));

LDAP_F( char * )
ldap_pvt_str2lower LDAP_P(( char *str ));

LDAP_F( struct berval * )
ldap_pvt_str2upperbv LDAP_P(( char *str, struct berval *bv ));

LDAP_F( struct berval * )
ldap_pvt_str2lowerbv LDAP_P(( char *str, struct berval *bv ));

/* tls.c */
LDAP_F (int) ldap_int_tls_config LDAP_P(( struct ldap *ld,
	int option, const char *arg ));
LDAP_F (int) ldap_pvt_tls_get_option LDAP_P(( struct ldap *ld,
	int option, void *arg ));
LDAP_F (int) ldap_pvt_tls_set_option LDAP_P(( struct ldap *ld,
	int option, void *arg ));

LDAP_F (void) ldap_pvt_tls_destroy LDAP_P(( void ));
LDAP_F (int) ldap_pvt_tls_init LDAP_P(( void ));
LDAP_F (int) ldap_pvt_tls_init_def_ctx LDAP_P(( void ));
LDAP_F (int) ldap_pvt_tls_accept LDAP_P(( Sockbuf *sb, void *ctx_arg ));
LDAP_F (int) ldap_pvt_tls_inplace LDAP_P(( Sockbuf *sb ));
LDAP_F (void *) ldap_pvt_tls_sb_ctx LDAP_P(( Sockbuf *sb ));

LDAP_F (int) ldap_pvt_tls_init_default_ctx LDAP_P(( void ));

typedef int LDAPDN_rewrite_dummy LDAP_P (( void *dn, unsigned flags ));

LDAP_F (int) ldap_pvt_tls_get_my_dn LDAP_P(( void *ctx, struct berval *dn,
	LDAPDN_rewrite_dummy *func, unsigned flags ));
LDAP_F (int) ldap_pvt_tls_get_peer_dn LDAP_P(( void *ctx, struct berval *dn,
	LDAPDN_rewrite_dummy *func, unsigned flags ));
LDAP_F (int) ldap_pvt_tls_get_strength LDAP_P(( void *ctx ));

LDAP_END_DECL

#include "ldap_pvt_uc.h"

#endif

