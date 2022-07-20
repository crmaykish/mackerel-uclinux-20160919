/* $OpenLDAP: pkg/ldap/servers/slapd/proto-slap.h,v 1.329.2.27 2003/05/07 22:29:11 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
#ifndef PROTO_SLAP_H
#define PROTO_SLAP_H

#include <ldap_cdefs.h>
#include "ldap_pvt.h"

LDAP_BEGIN_DECL

/*
 * acl.c
 */
LDAP_SLAPD_F (int) access_allowed LDAP_P((
	Backend *be, Connection *conn, Operation *op,
	Entry *e, AttributeDescription *desc, struct berval *val,
	slap_access_t access,
	AccessControlState *state ));
LDAP_SLAPD_F (int) acl_check_modlist LDAP_P((
	Backend *be, Connection *conn, Operation *op,
	Entry *e, Modifications *ml ));

LDAP_SLAPD_F (void) acl_append( AccessControl **l, AccessControl *a );

/*
 * aclparse.c
 */
LDAP_SLAPD_F (void) parse_acl LDAP_P(( Backend *be,
	const char *fname, int lineno,
	int argc, char **argv ));

LDAP_SLAPD_F (char *) access2str LDAP_P(( slap_access_t access ));
LDAP_SLAPD_F (slap_access_t) str2access LDAP_P(( const char *str ));

#define ACCESSMASK_MAXLEN	sizeof("unknown (+wrscan)")
LDAP_SLAPD_F (char *) accessmask2str LDAP_P(( slap_mask_t mask, char* ));
LDAP_SLAPD_F (slap_mask_t) str2accessmask LDAP_P(( const char *str ));
LDAP_SLAPD_F (void) acl_destroy LDAP_P(( AccessControl*, AccessControl* ));
LDAP_SLAPD_F (void) acl_free LDAP_P(( AccessControl *a ));

/*
 * ad.c
 */
LDAP_SLAPD_F (int) slap_str2ad LDAP_P((
	const char *,
	AttributeDescription **ad,
	const char **text ));

LDAP_SLAPD_F (int) slap_bv2ad LDAP_P((
	struct berval *bv,
	AttributeDescription **ad,
	const char **text ));

LDAP_SLAPD_F (void) ad_destroy LDAP_P(( AttributeDescription * ));

#define ad_cmp(l,r)	(((l)->ad_cname.bv_len < (r)->ad_cname.bv_len) \
	? -1 : (((l)->ad_cname.bv_len > (r)->ad_cname.bv_len) \
		? 1 : strcasecmp((l)->ad_cname.bv_val, (r)->ad_cname.bv_val )))

LDAP_SLAPD_F (int) is_ad_subtype LDAP_P((
	AttributeDescription *sub,
	AttributeDescription *super ));

LDAP_SLAPD_F (int) ad_inlist LDAP_P((
	AttributeDescription *desc,
	AttributeName *attrs ));

LDAP_SLAPD_F (int) slap_str2undef_ad LDAP_P((
	const char *,
	AttributeDescription **ad,
	const char **text ));

LDAP_SLAPD_F (int) slap_bv2undef_ad LDAP_P((
	struct berval *bv,
	AttributeDescription **ad,
	const char **text ));

LDAP_SLAPD_F (AttributeDescription *) ad_find_tags LDAP_P((
	AttributeType *type,
	struct berval *tags ));

LDAP_SLAPD_F (AttributeName *) str2anlist LDAP_P(( AttributeName *an,
	char *str, const char *brkstr ));
LDAP_SLAPD_F (int) an_find LDAP_P(( AttributeName *a, struct berval *s ));
LDAP_SLAPD_F (int) ad_define_option LDAP_P(( const char *name,
	const char *fname, int lineno ));

/*
 * add.c
 */
LDAP_SLAPD_F (int) slap_mods2entry LDAP_P(( Modifications *mods, Entry **e,
	int repl_user, const char **text, char *textbuf, size_t textlen ));

/*
 * at.c
 */
LDAP_SLAPD_F (void) at_config LDAP_P((
	const char *fname, int lineno,
	int argc, char **argv ));
LDAP_SLAPD_F (AttributeType *) at_find LDAP_P((
	const char *name ));
LDAP_SLAPD_F (AttributeType *) at_bvfind LDAP_P((
	struct berval *name ));
LDAP_SLAPD_F (int) at_find_in_list LDAP_P((
	AttributeType *sat, AttributeType **list ));
LDAP_SLAPD_F (int) at_append_to_list LDAP_P((
	AttributeType *sat, AttributeType ***listp ));
LDAP_SLAPD_F (int) at_delete_from_list LDAP_P((
	int pos, AttributeType ***listp ));
LDAP_SLAPD_F (int) at_schema_info LDAP_P(( Entry *e ));
LDAP_SLAPD_F (int) at_add LDAP_P((
	LDAPAttributeType *at, const char **err ));
LDAP_SLAPD_F (void) at_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) is_at_subtype LDAP_P((
	AttributeType *sub,
	AttributeType *super ));

LDAP_SLAPD_F (int) is_at_syntax LDAP_P((
	AttributeType *at,
	const char *oid ));

LDAP_SLAPD_F (int) at_start LDAP_P(( AttributeType **at ));
LDAP_SLAPD_F (int) at_next LDAP_P(( AttributeType **at ));

/*
 * attr.c
 */
LDAP_SLAPD_F (void) attr_free LDAP_P(( Attribute *a ));
LDAP_SLAPD_F (Attribute *) attr_dup LDAP_P(( Attribute *a ));

LDAP_SLAPD_F (int) attr_merge LDAP_P(( Entry *e,
	AttributeDescription *desc,
	BerVarray vals ));
LDAP_SLAPD_F (int) attr_merge_one LDAP_P(( Entry *e,
	AttributeDescription *desc,
	struct berval *val ));
LDAP_SLAPD_F (Attribute *) attrs_find LDAP_P((
	Attribute *a, AttributeDescription *desc ));
LDAP_SLAPD_F (Attribute *) attr_find LDAP_P((
	Attribute *a, AttributeDescription *desc ));
LDAP_SLAPD_F (int) attr_delete LDAP_P((
	Attribute **attrs, AttributeDescription *desc ));

LDAP_SLAPD_F (void) attrs_free LDAP_P(( Attribute *a ));
LDAP_SLAPD_F (Attribute *) attrs_dup LDAP_P(( Attribute *a ));


/*
 * ava.c
 */
LDAP_SLAPD_F (int) get_ava LDAP_P((
	BerElement *ber,
	AttributeAssertion **ava,
	unsigned usage,
	const char **text ));
LDAP_SLAPD_F (void) ava_free LDAP_P((
	AttributeAssertion *ava,
	int freeit ));

/*
 * backend.c
 */
LDAP_SLAPD_F (int) backend_init LDAP_P((void));
LDAP_SLAPD_F (int) backend_add LDAP_P((BackendInfo *aBackendInfo));
LDAP_SLAPD_F (int) backend_num LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_startup LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_sync LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_shutdown LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_destroy LDAP_P((void));

LDAP_SLAPD_F (BackendInfo *) backend_info LDAP_P(( const char *type ));
LDAP_SLAPD_F (BackendDB *) backend_db_init LDAP_P(( const char *type ));

LDAP_SLAPD_F (BackendDB *) select_backend LDAP_P((
	struct berval * dn,
	int manageDSAit,
	int noSubordinates ));

LDAP_SLAPD_F (int) be_issuffix LDAP_P(( Backend *be,
	struct berval *suffix ));
LDAP_SLAPD_F (int) be_isroot LDAP_P(( Backend *be,
	struct berval *ndn ));
LDAP_SLAPD_F (int) be_isroot_pw LDAP_P(( Backend *be,
	Connection *conn, struct berval *ndn, struct berval *cred ));
LDAP_SLAPD_F (int) be_isupdate LDAP_P(( Backend *be, struct berval *ndn ));
LDAP_SLAPD_F (struct berval *) be_root_dn LDAP_P(( Backend *be ));
LDAP_SLAPD_F (int) be_entry_release_rw LDAP_P((
	BackendDB *be, Connection *c, Operation *o, Entry *e, int rw ));
#define be_entry_release_r( be, c, o, e ) be_entry_release_rw( be, c, o, e, 0 )
#define be_entry_release_w( be, c, o, e ) be_entry_release_rw( be, c, o, e, 1 )

LDAP_SLAPD_F (int) backend_unbind LDAP_P((Connection *conn, Operation *op));

LDAP_SLAPD_F( int )	backend_check_restrictions LDAP_P((
	BackendDB *be,
	Connection *conn,
	Operation *op,
	struct berval *opdata,
	const char **text ));

LDAP_SLAPD_F( int )	backend_check_referrals LDAP_P((
	BackendDB *be,
	Connection *conn,
	Operation *op,
	struct berval *dn,
	struct berval *ndn ));

LDAP_SLAPD_F (int) backend_connection_init LDAP_P((Connection *conn));
LDAP_SLAPD_F (int) backend_connection_destroy LDAP_P((Connection *conn));

LDAP_SLAPD_F (int) backend_group LDAP_P((BackendDB *be,
	Connection *conn,
	Operation *op,
	Entry *target,
	struct berval *gr_ndn,
	struct berval *op_ndn,
	ObjectClass *group_oc,
	AttributeDescription *group_at
));

LDAP_SLAPD_F (int) backend_attribute LDAP_P((BackendDB *be,
	Connection *conn,
	Operation *op,
	Entry *target,
	struct berval *entry_ndn,
	AttributeDescription *entry_at,
	BerVarray *vals
));

LDAP_SLAPD_F (Attribute *) backend_operational(
	BackendDB *,
	Connection *conn,
	Operation *op,
	Entry *e,
	AttributeName *attrs,
	int opattrs );

/*
 * backglue.c
 */

LDAP_SLAPD_F (int) glue_back_initialize( BackendInfo *bi );
LDAP_SLAPD_F (int) glue_sub_init( void );

/*
 * ch_malloc.c
 */
#ifdef CSRIMALLOC
#define ch_malloc malloc
#define ch_realloc realloc
#define ch_calloc calloc
#define ch_strdup strdup
#define ch_free free

#else
LDAP_SLAPD_F (void *) ch_malloc LDAP_P(( ber_len_t size ));
LDAP_SLAPD_F (void *) ch_realloc LDAP_P(( void *block, ber_len_t size ));
LDAP_SLAPD_F (void *) ch_calloc LDAP_P(( ber_len_t nelem, ber_len_t size ));
LDAP_SLAPD_F (char *) ch_strdup LDAP_P(( const char *string ));
LDAP_SLAPD_F (void) ch_free LDAP_P(( void * ));

#ifndef CH_FREE
#undef free
#define free ch_free
#endif
#endif

/*
 * controls.c
 */
LDAP_SLAPD_F (int) get_ctrls LDAP_P((
	Connection *co,
	Operation *op,
	int senderrors ));

LDAP_SLAPD_F (char *) get_supported_ctrl LDAP_P((int index));

LDAP_SLAPD_F (slap_mask_t) get_supported_ctrl_mask LDAP_P((int index));

/*
 * config.c
 */
LDAP_SLAPD_F (int) read_config LDAP_P(( const char *fname, int depth ));
LDAP_SLAPD_F (void) config_destroy LDAP_P ((void));

/*
 * connection.c
 */
LDAP_SLAPD_F (int) connections_init LDAP_P((void));
LDAP_SLAPD_F (int) connections_shutdown LDAP_P((void));
LDAP_SLAPD_F (int) connections_destroy LDAP_P((void));
LDAP_SLAPD_F (int) connections_timeout_idle LDAP_P((time_t));

LDAP_SLAPD_F (long) connection_init LDAP_P((
	ber_socket_t s,
	Listener* url,
	const char* dnsname,
	const char* peername,
	int use_tls,
	slap_ssf_t ssf,
	const char *id ));

LDAP_SLAPD_F (void) connection_closing LDAP_P(( Connection *c ));
LDAP_SLAPD_F (int) connection_state_closing LDAP_P(( Connection *c ));
LDAP_SLAPD_F (const char *) connection_state2str LDAP_P(( int state ))
	LDAP_GCCATTR((const));

LDAP_SLAPD_F (int) connection_write LDAP_P((ber_socket_t s));
LDAP_SLAPD_F (int) connection_read LDAP_P((ber_socket_t s));

LDAP_SLAPD_F (unsigned long) connections_nextid(void);

LDAP_SLAPD_F (Connection *) connection_first LDAP_P(( ber_socket_t * ));
LDAP_SLAPD_F (Connection *) connection_next LDAP_P((
	Connection *, ber_socket_t *));
LDAP_SLAPD_F (void) connection_done LDAP_P((Connection *));

LDAP_SLAPD_F (void) connection2anonymous LDAP_P((Connection *));

/*
 * cr.c
 */
LDAP_SLAPD_F (int) cr_schema_info( Entry *e );

LDAP_SLAPD_F (int) cr_add LDAP_P((
	LDAPContentRule *oc,
	int user,
	const char **err));
LDAP_SLAPD_F (void) cr_destroy LDAP_P(( void ));

LDAP_SLAPD_F (ContentRule *) cr_find LDAP_P((
	const char *crname));
LDAP_SLAPD_F (ContentRule *) cr_bvfind LDAP_P((
	struct berval *crname));

/*
 * daemon.c
 */
LDAP_SLAPD_F (void) slapd_add_internal(ber_socket_t s);
LDAP_SLAPD_F (int) slapd_daemon_init( const char *urls );
LDAP_SLAPD_F (int) slapd_daemon_destroy(void);
LDAP_SLAPD_F (int) slapd_daemon(void);
LDAP_SLAPD_F (Listener **)	slapd_get_listeners LDAP_P((void));
LDAP_SLAPD_F (void) slapd_remove LDAP_P((ber_socket_t s, int wake));

LDAP_SLAPD_F (RETSIGTYPE) slap_sig_shutdown LDAP_P((int sig));
LDAP_SLAPD_F (RETSIGTYPE) slap_sig_wake LDAP_P((int sig));

LDAP_SLAPD_F (void) slapd_set_write LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (void) slapd_clr_write LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (void) slapd_set_read LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (void) slapd_clr_read LDAP_P((ber_socket_t s, int wake));

/*
 * dn.c
 */

#define dn_match(dn1, dn2) 	( ber_bvcmp((dn1), (dn2)) == 0 )
#define bvmatch(bv1, bv2)	( ((bv1)->bv_len == (bv2)->bv_len) && (memcmp((bv1)->bv_val, (bv2)->bv_val, (bv1)->bv_len) == 0) )

LDAP_SLAPD_V( const struct berval ) slap_empty_bv;

LDAP_SLAPD_F (int) dnValidate LDAP_P((
	Syntax *syntax, 
	struct berval *val ));

LDAP_SLAPD_F (int) dnNormalize LDAP_P((
	Syntax *syntax, 
	struct berval *val, 
	struct berval **normalized ));

LDAP_SLAPD_F (int) dnNormalize2 LDAP_P((
	Syntax *syntax, 
	struct berval *val, 
	struct berval *normalized ));

LDAP_SLAPD_F (int) dnPretty LDAP_P(( 
	Syntax *syntax, 
	struct berval *val, 
	struct berval **pretty ));

LDAP_SLAPD_F (int) dnPretty2 LDAP_P(( 
	Syntax *syntax, 
	struct berval *val, 
	struct berval *pretty ));

LDAP_SLAPD_F (int) dnPrettyNormal LDAP_P(( 
	Syntax *syntax, 
	struct berval *val, 
	struct berval *pretty,
	struct berval *normal ));

LDAP_SLAPD_F (int) dnMatch LDAP_P(( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue ));

LDAP_SLAPD_F (int) dnIsSuffix LDAP_P((
	const struct berval *dn, const struct berval *suffix ));

LDAP_SLAPD_F (int) dnExtractRdn LDAP_P((
	struct berval *dn, struct berval *rdn ));

LDAP_SLAPD_F (int) rdnValidate LDAP_P(( struct berval * rdn ));

LDAP_SLAPD_F (int) dn_rdnlen LDAP_P(( Backend *be, struct berval *dn ));

LDAP_SLAPD_F (void) build_new_dn LDAP_P((
	struct berval * new_dn,
	struct berval * parent_dn,
	struct berval * newrdn ));

LDAP_SLAPD_F (void) dnParent LDAP_P(( struct berval *dn, struct berval *pdn ));

LDAP_SLAPD_F (int) dnX509normalize LDAP_P(( void *x509_name, struct berval *out ));

LDAP_SLAPD_F (int) dnX509peerNormalize LDAP_P(( void *ssl, struct berval *dn ));

LDAP_SLAPD_F (int) dnPrettyNormalDN LDAP_P(( Syntax *syntax, struct berval *val, LDAPDN **dn, int flags ));
#define dnPrettyDN(syntax, val, dn) \
	dnPrettyNormalDN((syntax),(val),(dn), SLAP_LDAPDN_PRETTY)
#define dnNormalDN(syntax, val, dn) \
	dnPrettyNormalDN((syntax),(val),(dn), 0)


/*
 * entry.c
 */
LDAP_SLAPD_V (const Entry) slap_entry_root;

LDAP_SLAPD_F (int) entry_destroy LDAP_P((void));

LDAP_SLAPD_F (Entry *) str2entry LDAP_P(( char	*s ));
LDAP_SLAPD_F (char *) entry2str LDAP_P(( Entry *e, int *len ));

LDAP_SLAPD_F (int) entry_decode LDAP_P(( struct berval *bv, Entry **e ));
LDAP_SLAPD_F (int) entry_encode LDAP_P(( Entry *e, struct berval *bv ));

LDAP_SLAPD_F (void) entry_free LDAP_P(( Entry *e ));
LDAP_SLAPD_F (int) entry_cmp LDAP_P(( Entry *a, Entry *b ));
LDAP_SLAPD_F (int) entry_dn_cmp LDAP_P(( const void *v_a, const void *v_b ));
LDAP_SLAPD_F (int) entry_id_cmp LDAP_P(( const void *v_a, const void *v_b ));

/*
 * extended.c
 */
typedef int (SLAP_EXTOP_MAIN_FN) LDAP_P((
	Connection *conn, Operation *op,
	const char * reqoid,
	struct berval * reqdata,
	char ** rspoid,
	struct berval ** rspdata,
	LDAPControl *** rspctrls,
	const char ** text,
	BerVarray *refs ));

typedef int (SLAP_EXTOP_GETOID_FN) LDAP_P((
	int index, char *oid, int blen ));

LDAP_SLAPD_F (int) load_extop LDAP_P((
	const char *ext_oid,
	SLAP_EXTOP_MAIN_FN *ext_main ));

LDAP_SLAPD_F (int) extops_init LDAP_P(( void ));

LDAP_SLAPD_F (int) extops_kill LDAP_P(( void ));

LDAP_SLAPD_F (struct berval *) get_supported_extop LDAP_P((int index));

/*
 *  * cancel.c
 *   */
LDAP_SLAPD_F ( SLAP_EXTOP_MAIN_FN ) cancel_extop;

/*
 * filter.c
 */
LDAP_SLAPD_F (int) get_filter LDAP_P((
	Connection *conn,
	BerElement *ber,
	Filter **filt,
	const char **text ));

LDAP_SLAPD_F (void) filter_free LDAP_P(( Filter *f ));
LDAP_SLAPD_F (void) filter2bv LDAP_P(( Filter *f, struct berval *bv ));

LDAP_SLAPD_F (int) get_vrFilter LDAP_P(( Connection *conn, BerElement *ber,
	ValuesReturnFilter **f,
	const char **text ));

LDAP_SLAPD_F (void) vrFilter_free LDAP_P(( ValuesReturnFilter *f ));
LDAP_SLAPD_F (void) vrFilter2bv LDAP_P(( ValuesReturnFilter *f, struct berval *fstr ));

LDAP_SLAPD_F (int) filter_has_subordinates LDAP_P(( Filter *filter ));

/*
 * filterentry.c
 */

LDAP_SLAPD_F (int) test_filter LDAP_P((
	Backend *be, Connection *conn, Operation *op,
	Entry *e, Filter *f ));

/*
 * index.c
 */
LDAP_SLAPD_F (int) slap_str2index LDAP_P(( const char *str, slap_mask_t *idx ));

/*
 * init.c
 */
LDAP_SLAPD_F (int)	slap_init LDAP_P((int mode, const char* name));
LDAP_SLAPD_F (int)	slap_startup LDAP_P(( Backend *be ));
LDAP_SLAPD_F (int)	slap_shutdown LDAP_P(( Backend *be ));
LDAP_SLAPD_F (int)	slap_destroy LDAP_P((void));

LDAP_SLAPD_V (char *)	slap_known_controls[];

/*
 * kerberos.c
 */
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
LDAP_SLAPD_V (char *)	ldap_srvtab;
LDAP_SLAPD_V (int)	krbv4_ldap_auth();
#endif

/*
 * limits.c
 */
LDAP_SLAPD_F (int) get_limits LDAP_P((
	Backend *be, struct berval *ndn,
	struct slap_limits_set **limit ));
LDAP_SLAPD_F (int) parse_limits LDAP_P((
	Backend *be, const char *fname, int lineno,
	int argc, char **argv ));
LDAP_SLAPD_F (int) parse_limit LDAP_P(( const char *arg, 
	struct slap_limits_set *limit ));

/*
 * lock.c
 */
LDAP_SLAPD_F (FILE *) lock_fopen LDAP_P(( const char *fname,
	const char *type, FILE **lfp ));
LDAP_SLAPD_F (int) lock_fclose LDAP_P(( FILE *fp, FILE *lfp ));

/*
 * matchedValues.c
 */
LDAP_SLAPD_F (int) filter_matched_values( 
	Backend		*be,
	Connection	*conn,
	Operation	*op,
	Attribute	*a,
	char		***e_flags );

/*
 * modrdn.c
 */
LDAP_SLAPD_F (int) slap_modrdn2mods(
	Backend		*be,
	Connection	*conn,
	Operation	*op,
	Entry		*e,
	LDAPRDN		*oldrdn,
	LDAPRDN		*newrdn,
	int		deleteoldrdn,
	Modifications	**pmod );

/*
 * modify.c
 */
LDAP_SLAPD_F( int ) slap_mods_check(
	Modifications *ml,
	int update,
	const char **text,
	char *textbuf, size_t textlen );

LDAP_SLAPD_F( int ) slap_mods_opattrs(
	Backend *be,
	Operation *op,
	Modifications *mods,
	Modifications **modlist,
	const char **text,
	char *textbuf, size_t textlen );

/*
 * mods.c
 */
LDAP_SLAPD_F( int ) modify_check_duplicates(
	AttributeDescription *ad, MatchingRule *mr, 
	BerVarray vals, BerVarray mods, int permissive, 
	const char **text, char *textbuf, size_t textlen );
LDAP_SLAPD_F( int ) modify_add_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );
LDAP_SLAPD_F( int ) modify_delete_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );
LDAP_SLAPD_F( int ) modify_replace_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );

LDAP_SLAPD_F( void ) slap_mod_free( Modification *mod, int freeit );
LDAP_SLAPD_F( void ) slap_mods_free( Modifications *mods );
LDAP_SLAPD_F( void ) slap_modlist_free( LDAPModList *ml );

/*
 * module.c
 */
#ifdef SLAPD_MODULES

LDAP_SLAPD_F (int) module_init LDAP_P(( void ));
LDAP_SLAPD_F (int) module_kill LDAP_P(( void ));

LDAP_SLAPD_F (int) load_null_module(
	const void *module, const char *file_name);
LDAP_SLAPD_F (int) load_extop_module(
	const void *module, const char *file_name);

LDAP_SLAPD_F (int) module_load LDAP_P((
	const char* file_name,
	int argc, char *argv[] ));
LDAP_SLAPD_F (int) module_path LDAP_P(( const char* path ));

LDAP_SLAPD_F (void) *module_resolve LDAP_P((
	const void *module, const char *name));

#endif /* SLAPD_MODULES */

/* mr.c */
LDAP_SLAPD_F (MatchingRule *) mr_bvfind LDAP_P((struct berval *mrname));
LDAP_SLAPD_F (MatchingRule *) mr_find LDAP_P((const char *mrname));
LDAP_SLAPD_F (int) mr_add LDAP_P(( LDAPMatchingRule *mr,
	slap_mrule_defs_rec *def,
	MatchingRule * associated,
	const char **err ));
LDAP_SLAPD_F (void) mr_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) register_matching_rule LDAP_P((
	slap_mrule_defs_rec *def ));

LDAP_SLAPD_F (void) mru_destroy LDAP_P(( void ));
LDAP_SLAPD_F (int) matching_rule_use_init LDAP_P(( void ));

LDAP_SLAPD_F (int) mr_schema_info( Entry *e );
LDAP_SLAPD_F (int) mru_schema_info( Entry *e );

LDAP_SLAPD_F (int) mr_usable_with_at( MatchingRule *mr,
	AttributeType *at );

/*
 * mra.c
 */
LDAP_SLAPD_F (int) get_mra LDAP_P((
	BerElement *ber,
	MatchingRuleAssertion **mra,
	const char **text ));
LDAP_SLAPD_F (void) mra_free LDAP_P((
	MatchingRuleAssertion *mra,
	int freeit ));

/* oc.c */
LDAP_SLAPD_F (int) oc_add LDAP_P((
	LDAPObjectClass *oc,
	int user,
	const char **err));
LDAP_SLAPD_F (void) oc_destroy LDAP_P(( void ));

LDAP_SLAPD_F (ObjectClass *) oc_find LDAP_P((
	const char *ocname));
LDAP_SLAPD_F (ObjectClass *) oc_bvfind LDAP_P((
	struct berval *ocname));
LDAP_SLAPD_F (int) is_object_subclass LDAP_P((
	ObjectClass *sup,
	ObjectClass *sub ));

LDAP_SLAPD_F (int) is_entry_objectclass LDAP_P((
	Entry *, ObjectClass *oc, int set_flags ));
#define is_entry_alias(e)		\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_ALIAS) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_alias, 1))
#define is_entry_referral(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_REFERRAL) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_referral, 1))
#define is_entry_subentry(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_SUBENTRY) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_subentry, 1))
#define is_entry_collectiveAttributeSubentry(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_collectiveAttributeSubentry, 1))
#define is_entry_dynamicObject(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_DYNAMICOBJECT) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_dynamicObject, 1))

LDAP_SLAPD_F (int) oc_schema_info( Entry *e );

/*
 * oidm.c
 */
LDAP_SLAPD_F(char *) oidm_find(char *oid);
LDAP_SLAPD_F (void) oidm_destroy LDAP_P(( void ));
LDAP_SLAPD_F (int) parse_oidm LDAP_P((
	const char *fname, int lineno, int argc, char **argv ));

/*
 * operation.c
 */
LDAP_SLAPD_F (void) slap_op_init LDAP_P(( void ));
LDAP_SLAPD_F (void) slap_op_destroy LDAP_P(( void ));
LDAP_SLAPD_F (void) slap_op_free LDAP_P(( Operation *op ));
LDAP_SLAPD_F (Operation *) slap_op_alloc LDAP_P((
	BerElement *ber, ber_int_t msgid,
	ber_tag_t tag, ber_int_t id ));

LDAP_SLAPD_F (int) slap_op_add LDAP_P(( Operation **olist, Operation *op ));
LDAP_SLAPD_F (int) slap_op_remove LDAP_P(( Operation **olist, Operation *op ));
LDAP_SLAPD_F (Operation *) slap_op_pop LDAP_P(( Operation **olist ));

/*
 * operational.c
 */
LDAP_SLAPD_F (Attribute *) slap_operational_subschemaSubentry( Backend *be );
LDAP_SLAPD_F (Attribute *) slap_operational_hasSubordinate( int has );

/*
 * passwd.c
 */
LDAP_SLAPD_F (SLAP_EXTOP_MAIN_FN) passwd_extop;

LDAP_SLAPD_F (int) slap_passwd_check(
	Connection			*conn,
	Attribute			*attr,
	struct berval		*cred );

LDAP_SLAPD_F (void) slap_passwd_generate( struct berval * );

LDAP_SLAPD_F (void) slap_passwd_hash(
	struct berval		*cred,
	struct berval		*hash );

LDAP_SLAPD_F (struct berval *) slap_passwd_return(
	struct berval		*cred );

LDAP_SLAPD_F (int) slap_passwd_parse(
	struct berval *reqdata,
	struct berval *id,
	struct berval *oldpass,
	struct berval *newpass,
	const char **text );

/*
 * phonetic.c
 */
LDAP_SLAPD_F (char *) phonetic LDAP_P(( char *s ));

/*
 * referral.c
 */
LDAP_SLAPD_F (int) validate_global_referral LDAP_P((
	const char *url ));

LDAP_SLAPD_F (BerVarray) get_entry_referrals LDAP_P((
	Backend *be, Connection *conn, Operation *op, Entry *e ));

LDAP_SLAPD_F (BerVarray) referral_rewrite LDAP_P((
	BerVarray refs,
	struct berval *base,
	struct berval *target,
	int scope ));

/*
 * repl.c
 */
LDAP_SLAPD_F (int) add_replica_info LDAP_P(( Backend *be,
	const char *host ));
LDAP_SLAPD_F (int) add_replica_suffix LDAP_P(( Backend *be,
	int nr, const char *suffix ));
LDAP_SLAPD_F (int) add_replica_attrs LDAP_P(( Backend *be,
	int nr, char *attrs, int exclude ));
LDAP_SLAPD_F (void) replog LDAP_P(( Backend *be, Operation *op,
	struct berval *dn, struct berval *ndn, void *change ));

/*
 * result.c
 */
LDAP_SLAPD_F (void) slap_send_ldap_result LDAP_P((
	Connection *conn, Operation *op,
	ber_int_t err, const char *matched, const char *text,
	BerVarray refs,
	LDAPControl **ctrls ));

LDAP_SLAPD_F (void) send_ldap_sasl LDAP_P((
	Connection *conn, Operation *op,
	ber_int_t err, const char *matched,
	const char *text,
	BerVarray refs,
	LDAPControl **ctrls,
	struct berval *cred ));

LDAP_SLAPD_F (void) send_ldap_disconnect LDAP_P((
	Connection *conn, Operation *op,
	ber_int_t err, const char *text ));

LDAP_SLAPD_F (void) slap_send_ldap_extended LDAP_P((
	Connection *conn, Operation *op,
	ber_int_t err, const char *matched,
	const char *text, BerVarray refs,
	const char *rspoid, struct berval *rspdata,
	LDAPControl **ctrls ));

LDAP_SLAPD_F (void) slap_send_ldap_intermediate_resp LDAP_P((
	Connection *conn, Operation *op,
	ber_int_t err, const char *matched,
	const char *text, BerVarray refs,
	const char *rspoid, struct berval *rspdata,
	LDAPControl **ctrls ));

LDAP_SLAPD_F (void) send_ldap_partial LDAP_P((
	Connection *conn, Operation *op,
	const char *rspoid, struct berval *rspdata,
	LDAPControl **ctrls ));

LDAP_SLAPD_F (void) slap_send_search_result LDAP_P((
	Connection *conn, Operation *op,
	ber_int_t err, const char *matched, const char *text,
	BerVarray refs,
	LDAPControl **ctrls,
	int nentries ));

LDAP_SLAPD_F (int) slap_send_search_reference LDAP_P((
	Backend *be, Connection *conn, Operation *op,
	Entry *e, BerVarray refs,
	LDAPControl **ctrls,
	BerVarray *v2refs ));

LDAP_SLAPD_F (int) slap_send_search_entry LDAP_P((
	Backend *be, Connection *conn, Operation *op,
	Entry *e, AttributeName *attrs, int attrsonly,
	LDAPControl **ctrls ));

LDAP_SLAPD_F (int) str2result LDAP_P(( char *s,
	int *code, char **matched, char **info ));

/*
 * root_dse.c
 */
LDAP_SLAPD_F (int) root_dse_info LDAP_P((
	Connection *conn,
	Entry **e,
	const char **text ));

LDAP_SLAPD_F (int) read_root_dse_file LDAP_P((
	const char *file));

/*
 * sasl.c
 */
LDAP_SLAPD_F (int) slap_sasl_init(void);
LDAP_SLAPD_F (char *) slap_sasl_secprops( const char * );
LDAP_SLAPD_F (int) slap_sasl_destroy(void);

LDAP_SLAPD_F (int) slap_sasl_open( Connection *c );
LDAP_SLAPD_F (char **) slap_sasl_mechs( Connection *c );

LDAP_SLAPD_F (int) slap_sasl_external( Connection *c,
	slap_ssf_t ssf,	/* relative strength of external security */
	const char *authid );	/* asserted authenication id */

LDAP_SLAPD_F (int) slap_sasl_reset( Connection *c );
LDAP_SLAPD_F (int) slap_sasl_close( Connection *c );

LDAP_SLAPD_F (int) slap_sasl_bind LDAP_P((
	Connection *conn, Operation *op, 
	struct berval *dn, struct berval *ndn,
	struct berval *cred,
	struct berval *edn, slap_ssf_t *ssf ));

LDAP_SLAPD_F (int) slap_sasl_setpass(
	Connection      *conn,
	Operation       *op,
	const char      *reqoid,
	struct berval   *reqdata,
	char            **rspoid,
	struct berval   **rspdata,
	LDAPControl     *** rspctrls,
	const char      **text );

LDAP_SLAPD_F (int) slap_sasl_config(
	int cargc,
	char **cargv,
	char *line,
	const char *fname,
	int lineno );

LDAP_SLAPD_F (int) slap_sasl_getdn( Connection *conn,
	char *id, int len,
	char *user_realm, struct berval *dn, int flags );

/*
 * saslauthz.c
 */
LDAP_SLAPD_F (void) slap_sasl2dn LDAP_P((
	Connection *conn,
	struct berval *saslname,
	struct berval *dn ));
LDAP_SLAPD_F (int) slap_sasl_authorized LDAP_P((
	Connection *conn,
	struct berval *authcid,
	struct berval *authzid ));
LDAP_SLAPD_F (int) slap_sasl_regexp_config LDAP_P((
	const char *match, const char *replace ));
LDAP_SLAPD_F (int) slap_sasl_setpolicy LDAP_P(( const char * ));
LDAP_SLAPD_F (slap_response) slap_cb_null_response;
LDAP_SLAPD_F (slap_sresult) slap_cb_null_sresult;
LDAP_SLAPD_F (slap_sendreference) slap_cb_null_sreference;


/*
 * schema.c
 */
LDAP_SLAPD_F (int) schema_info LDAP_P(( Entry **entry, const char **text ));

/*
 * schema_check.c
 */
LDAP_SLAPD_F( int ) oc_check_allowed(
	AttributeType *type,
	BerVarray oclist,
	ObjectClass *sc );

LDAP_SLAPD_F( int ) structural_class(
	BerVarray ocs,
	struct berval *scbv,
	ObjectClass **sc,
	const char **text,
	char *textbuf, size_t textlen );

LDAP_SLAPD_F( int ) entry_schema_check(
	Backend *be, Entry *e, Attribute *attrs,
	const char** text,
	char *textbuf, size_t textlen );

LDAP_SLAPD_F( int ) mods_structural_class(
	Modifications *mods,
	struct berval *oc,
	const char** text,
	char *textbuf, size_t textlen );

/*
 * schema_init.c
 */
LDAP_SLAPD_V( int ) schema_init_done;
LDAP_SLAPD_F (int) slap_schema_init LDAP_P((void));
LDAP_SLAPD_F (void) schema_destroy LDAP_P(( void ));

LDAP_SLAPD_F( int ) octetStringIndexer(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp );

LDAP_SLAPD_F( int ) octetStringFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertValue,
	BerVarray *keysp );

/*
 * schema_prep.c
 */
LDAP_SLAPD_V( struct slap_internal_schema ) slap_schema;
LDAP_SLAPD_F (int) slap_schema_load LDAP_P((void));
LDAP_SLAPD_F (int) slap_schema_check LDAP_P((void));

/*
 * schemaparse.c
 */
LDAP_SLAPD_F( int ) slap_valid_descr( const char * );

LDAP_SLAPD_F (int) parse_cr LDAP_P((
	const char *fname, int lineno, char *line, char **argv ));
LDAP_SLAPD_F (int) parse_oc LDAP_P((
	const char *fname, int lineno, char *line, char **argv ));
LDAP_SLAPD_F (int) parse_at LDAP_P((
	const char *fname, int lineno, char *line, char **argv ));
LDAP_SLAPD_F (char *) scherr2str LDAP_P((int code)) LDAP_GCCATTR((const));
LDAP_SLAPD_F (int) dscompare LDAP_P(( const char *s1, const char *s2del,
	char delim ));

/*
 * starttls.c
 */
LDAP_SLAPD_F (SLAP_EXTOP_MAIN_FN) starttls_extop;

/*
 * str2filter.c
 */
LDAP_SLAPD_F (Filter *) str2filter LDAP_P(( const char *str ));

/* syntax.c */
LDAP_SLAPD_F (Syntax *) syn_find LDAP_P((
	const char *synname ));
LDAP_SLAPD_F (Syntax *) syn_find_desc LDAP_P((
	const char *syndesc, int *slen ));
LDAP_SLAPD_F (int) syn_add LDAP_P((
	LDAPSyntax *syn,
	slap_syntax_defs_rec *def,
	const char **err ));
LDAP_SLAPD_F (void) syn_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) register_syntax LDAP_P((
	slap_syntax_defs_rec *def ));

LDAP_SLAPD_F (int) syn_schema_info( Entry *e );

/*
 * user.c
 */
#if defined(HAVE_PWD_H) && defined(HAVE_GRP_H)
LDAP_SLAPD_F (void) slap_init_user LDAP_P(( char *username, char *groupname ));
#endif

/*
 * value.c
 */
LDAP_SLAPD_F (int) value_validate LDAP_P((
	MatchingRule *mr,
	struct berval *in,
	const char ** text ));
LDAP_SLAPD_F (int) value_normalize LDAP_P((
	AttributeDescription *ad,
	unsigned usage,
	struct berval *in,
	struct berval *out,
	const char ** text ));
LDAP_SLAPD_F (int) value_validate_normalize LDAP_P((
	AttributeDescription *ad,
	unsigned usage,
	struct berval *in,
	struct berval *out,
	const char ** text ));
LDAP_SLAPD_F (int) value_match LDAP_P((
	int *match,
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned flags,
	struct berval *v1,
	void *v2,
	const char ** text ));
#define value_find(ad,values,value) (value_find_ex((ad),0,(values),(value)))
LDAP_SLAPD_F (int) value_find_ex LDAP_P((
	AttributeDescription *ad,
	unsigned flags,
	BerVarray values,
	struct berval *value ));
LDAP_SLAPD_F (int) value_add LDAP_P((
	BerVarray *vals,
	BerVarray addvals ));
LDAP_SLAPD_F (int) value_add_one LDAP_P((
	BerVarray *vals,
	struct berval *addval ));

/* assumes (x) > (y) returns 1 if true, 0 otherwise */
#define SLAP_PTRCMP(x, y) ((x) < (y) ? -1 : (x) > (y))

/*
 * Other...
 */
LDAP_SLAPD_V(unsigned) num_subordinates;

LDAP_SLAPD_V (ber_len_t) sockbuf_max_incoming;
LDAP_SLAPD_V (ber_len_t) sockbuf_max_incoming_auth;
LDAP_SLAPD_V (int)		slap_conn_max_pending;
LDAP_SLAPD_V (int)		slap_conn_max_pending_auth;

LDAP_SLAPD_V (slap_mask_t)	global_restrictops;
LDAP_SLAPD_V (slap_mask_t)	global_allows;
LDAP_SLAPD_V (slap_mask_t)	global_disallows;
LDAP_SLAPD_V (slap_mask_t)	global_requires;
LDAP_SLAPD_V (slap_ssf_set_t)	global_ssf_set;

LDAP_SLAPD_V (BerVarray)		default_referral;
LDAP_SLAPD_V (char *)		replogfile;
LDAP_SLAPD_V (const char) 	Versionstr[];
LDAP_SLAPD_V (struct slap_limits_set)		deflimit;

LDAP_SLAPD_V (slap_access_t)	global_default_access;
LDAP_SLAPD_V (int)		global_gentlehup;
LDAP_SLAPD_V (int)		global_idletimeout;
LDAP_SLAPD_V (int)		global_schemacheck;
LDAP_SLAPD_V (char *)	global_host;
LDAP_SLAPD_V (char *)	global_realm;
LDAP_SLAPD_V (char *)	default_passwd_hash;
LDAP_SLAPD_V (int)		lber_debug;
LDAP_SLAPD_V (int)		ldap_syslog;
LDAP_SLAPD_V (struct berval)	default_search_base;
LDAP_SLAPD_V (struct berval)	default_search_nbase;

LDAP_SLAPD_V (struct berval)	global_schemadn;
LDAP_SLAPD_V (struct berval)	global_schemandn;

LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	num_sent_mutex;
LDAP_SLAPD_V (unsigned long)		num_bytes_sent;
LDAP_SLAPD_V (unsigned long)		num_pdu_sent;
LDAP_SLAPD_V (unsigned long)		num_entries_sent;
LDAP_SLAPD_V (unsigned long)		num_refs_sent;

LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	num_ops_mutex;
LDAP_SLAPD_V (unsigned long)		num_ops_completed;
LDAP_SLAPD_V (unsigned long)		num_ops_initiated;
#ifdef SLAPD_MONITOR
LDAP_SLAPD_V (unsigned long)		num_ops_completed_[SLAP_OP_LAST];
LDAP_SLAPD_V (unsigned long)		num_ops_initiated_[SLAP_OP_LAST];
#endif /* SLAPD_MONITOR */

LDAP_SLAPD_V (char *)		slapd_pid_file;
LDAP_SLAPD_V (char *)		slapd_args_file;
LDAP_SLAPD_V (time_t)		starttime;

/* use time(3) -- no mutex */
#define slap_get_time()	time( NULL )

LDAP_SLAPD_V (ldap_pvt_thread_pool_t)	connection_pool;
LDAP_SLAPD_V (int)			connection_pool_max;

LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	entry2str_mutex;
LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	replog_mutex;

#if defined( SLAPD_CRYPT ) || defined( SLAPD_SPASSWD )
LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	passwd_mutex;
#endif
LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	gmtime_mutex;

LDAP_SLAPD_V (AccessControl *) global_acl;

LDAP_SLAPD_V (ber_socket_t)	dtblsize;

LDAP_SLAPD_V (int)		use_reverse_lookup;

LDAP_SLAPD_V (struct berval)	AllUser;
LDAP_SLAPD_V (struct berval)	AllOper;
LDAP_SLAPD_V (struct berval)	NoAttrs;

/*
 * operations
 */
LDAP_SLAPD_F (int) do_abandon LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_add LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_bind LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_compare LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_delete LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_modify LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_modrdn LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_search LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_unbind LDAP_P((Connection *conn, Operation *op));
LDAP_SLAPD_F (int) do_extended LDAP_P((Connection *conn, Operation *op));

LDAP_END_DECL

#endif /* PROTO_SLAP_H */

