/* $OpenLDAP: pkg/ldap/servers/slurpd/proto-slurp.h,v 1.8.2.2 2003/03/03 17:10:11 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
#ifndef _PROTO_SLURP
#define _PROTO_SLURP

#include <ldap_cdefs.h>

struct globals;

/* admin.c */
RETSIGTYPE do_admin	LDAP_P((int sig));

/* args.c */
int doargs	LDAP_P((int argc, char **argv, struct globals *g));

/* ch_malloc.c */
#ifdef CSRIMALLOC
#define ch_malloc malloc
#define ch_realloc realloc
#define ch_calloc calloc
#define ch_free free
#else
void *ch_malloc	LDAP_P((ber_len_t size));
void *ch_realloc	LDAP_P((void *block, ber_len_t size));
void *ch_calloc	LDAP_P((ber_len_t nelem, ber_len_t size));
void ch_free	LDAP_P((void *p));
#endif

/* config.c */
int slurpd_read_config	LDAP_P((char *fname));

/* ch_malloc.c */
void ch_free LDAP_P(( void *p ));

/* fm.c */
void *fm	LDAP_P((void *arg));
RETSIGTYPE do_nothing	LDAP_P((int i));
RETSIGTYPE slurp_set_shutdown LDAP_P((int));

/* globals.c */
extern struct globals *sglob;
extern int ldap_syslog;
extern int ldap_syslog_level;
extern int ldap_debug;
extern struct globals *init_globals	LDAP_P((void));

/* ldap_op.c */
int do_ldap	LDAP_P((Ri *ri, Re *re, char **errmsg));

/* lock.c */
FILE *lock_fopen	LDAP_P((const char *fname, const char *type, FILE **lfp));
int lock_fclose	LDAP_P((FILE *fp, FILE *lfp));
int acquire_lock	LDAP_P((const char *file, FILE **rfp, FILE **lfp));
int relinquish_lock	LDAP_P((const char *file, FILE *rfp, FILE *lfp));

/* reject.c */
void write_reject	LDAP_P((Ri *ri, Re *re, int lderr, char *errmsg));

/* replica.c */
int start_replica_thread	LDAP_P((Ri *ri));

/* replog.c */
int copy_replog	LDAP_P((char *src, char *dst));
int file_nonempty	LDAP_P((char *filename));

/* sanity.c */
int sanity	LDAP_P((void));

/* st.c */
int St_init	LDAP_P((St **st));

/* tsleep.c */
int tsleep	LDAP_P((time_t interval));
#if defined( HAVE_LWP )
void start_lwp_scheduler LDAP_P(( void ));
#endif

#endif /* _PROTO_SLURP */
