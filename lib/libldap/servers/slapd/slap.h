/* slap.h - stand alone ldap server include file */
/* $OpenLDAP: pkg/ldap/servers/slapd/slap.h,v 1.323.2.29 2003/05/16 00:38:09 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef _SLAP_H_
#define _SLAP_H_

#include "ldap_defaults.h"

#include <stdio.h>
#include <ac/stdlib.h>

#include <sys/types.h>
#include <ac/syslog.h>
#include <ac/regex.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/time.h>
#include <ac/param.h>

#include "avl.h"

#ifndef ldap_debug
#define ldap_debug slap_debug
#endif

#include "ldap_log.h"

#include <ldap.h>
#include <ldap_schema.h>

#include "ldap_pvt_thread.h"
#include "ldap_queue.h"

#ifdef LDAP_DEVEL
#define SLAP_EXTENDED_SCHEMA 1
#endif

LDAP_BEGIN_DECL
/*
 * SLAPD Memory allocation macros
 *
 * Unlike ch_*() routines, these routines do not assert() upon
 * allocation error.  They are intended to be used instead of
 * ch_*() routines where the caller has implemented proper
 * checking for and handling of allocation errors.
 *
 * Patches to convert ch_*() calls to SLAP_*() calls welcomed.
 */
#define SLAP_MALLOC(s)      ber_memalloc((s))
#define SLAP_CALLOC(n,s)    ber_memcalloc((n),(s))
#define SLAP_REALLOC(p,s)   ber_memrealloc((p),(s))
#define SLAP_FREE(p)        ber_memfree((p))
#define SLAP_VFREE(v)       ber_memvfree((void**)(v))
#define SLAP_STRDUP(s)      ber_strdup((s))
#define SLAP_STRNDUP(s,l)   ber_strndup((s),(l))

#ifdef f_next
#undef f_next /* name conflict between sys/file.h on SCO and struct filter */
#endif

#define SERVICE_NAME  OPENLDAP_PACKAGE "-slapd"
#define SLAPD_ANONYMOUS "cn=anonymous"

/* LDAPMod.mod_op value ===> Must be kept in sync with ldap.h!
 * This is a value used internally by the backends. It is needed to allow
 * adding values that already exist without getting an error as required by
 * modrdn when the new rdn was already an attribute value itself.
 */
#define SLAP_MOD_SOFTADD	0x1000

#define MAXREMATCHES (100)

#define SLAP_MAX_WORKER_THREADS		(16)

#define SLAP_SB_MAX_INCOMING_DEFAULT ((1<<18) - 1)
#define SLAP_SB_MAX_INCOMING_AUTH ((1<<24) - 1)

#define SLAP_CONN_MAX_PENDING_DEFAULT	100
#define SLAP_CONN_MAX_PENDING_AUTH	1000

#define SLAP_TEXT_BUFLEN (256)

/* psuedo error code indicating abandoned operation */
#define SLAPD_ABANDON (-1)

/* psuedo error code indicating disconnect */
#define SLAPD_DISCONNECT (-2)


/* We assume "C" locale, that is US-ASCII */
#define ASCII_SPACE(c)	( (c) == ' ' )
#define ASCII_LOWER(c)	( (c) >= 'a' && (c) <= 'z' )
#define ASCII_UPPER(c)	( (c) >= 'A' && (c) <= 'Z' )
#define ASCII_ALPHA(c)	( ASCII_LOWER(c) || ASCII_UPPER(c) )
#define ASCII_DIGIT(c)	( (c) >= '0' && (c) <= '9' )
#define ASCII_ALNUM(c)	( ASCII_ALPHA(c) || ASCII_DIGIT(c) )
#define ASCII_PRINTABLE(c) ( (c) >= ' ' && (c) <= '~' )

#define SLAP_NIBBLE(c) ((c)&0x0f)
#define SLAP_ESCAPE_CHAR ('\\')
#define SLAP_ESCAPE_LO(c) ( "0123456789ABCDEF"[SLAP_NIBBLE(c)] )
#define SLAP_ESCAPE_HI(c) ( SLAP_ESCAPE_LO((c)>>4) )

#define FILTER_ESCAPE(c) ( (c) == '*' || (c) == '\\' \
	|| (c) == '(' || (c) == ')' || !ASCII_PRINTABLE(c) )

#define DN_ESCAPE(c)	((c) == SLAP_ESCAPE_CHAR)
#define DN_SEPARATOR(c)	((c) == ',' || (c) == ';')
#define RDN_ATTRTYPEANDVALUE_SEPARATOR(c) ((c) == '+') /* RFC 2253 */
#define RDN_SEPARATOR(c) (DN_SEPARATOR(c) || RDN_ATTRTYPEANDVALUE_SEPARATOR(c))
#define RDN_NEEDSESCAPE(c)	((c) == '\\' || (c) == '"')

#define DESC_LEADCHAR(c)	( ASCII_ALPHA(c) )
#define DESC_CHAR(c)	( ASCII_ALNUM(c) || (c) == '-' )
#define OID_LEADCHAR(c)	( ASCII_DIGIT(c) )
#define OID_SEPARATOR(c)	( (c) == '.' )
#define OID_CHAR(c)	( OID_LEADCHAR(c) || OID_SEPARATOR(c) )

#define ATTR_LEADCHAR(c)	( DESC_LEADCHAR(c) || OID_LEADCHAR(c) )
#define ATTR_CHAR(c)	( DESC_CHAR((c)) || (c) == '.' )

#define AD_LEADCHAR(c)	( ATTR_CHAR(c) )
#define AD_CHAR(c)		( ATTR_CHAR(c) || (c) == ';' )

#define SLAP_NUMERIC(c) ( ASCII_DIGIT(c) || ASCII_SPACE(c) )

#define SLAP_PRINTABLE(c)	( ASCII_ALNUM(c) || (c) == '\'' || \
	(c) == '(' || (c) == ')' || (c) == '+' || (c) == ',' || \
	(c) == '-' || (c) == '.' || (c) == '/' || (c) == ':' || \
	(c) == '?' || (c) == ' ' || (c) == '=' )
#define SLAP_PRINTABLES(c)	( SLAP_PRINTABLE(c) || (c) == '$' )

/* must match in schema_init.c */
#define SLAPD_DN_SYNTAX			"1.3.6.1.4.1.1466.115.121.1.12"
#define SLAPD_NAMEUID_SYNTAX	"1.3.6.1.4.1.1466.115.121.1.34"
#define SLAPD_GROUP_ATTR		"member"
#define SLAPD_GROUP_CLASS		"groupOfNames"
#define SLAPD_ROLE_ATTR			"roleOccupant"
#define SLAPD_ROLE_CLASS		"organizationalRole"

#ifdef SLAPD_ACI_ENABLED
#define SLAPD_ACI_SYNTAX		"1.3.6.1.4.1.4203.666.2.1"
#endif

/* change this to "OpenLDAPset" */
#define SLAPD_ACI_SET_ATTR		"template"

#define SLAPD_TOP_OID			"2.5.6.0"

LDAP_SLAPD_V (int) slap_debug;

typedef unsigned long slap_mask_t;

/* Security Strength Factor */
typedef unsigned slap_ssf_t;

typedef struct slap_ssf_set {
	slap_ssf_t sss_ssf;
	slap_ssf_t sss_transport;
	slap_ssf_t sss_tls;
	slap_ssf_t sss_sasl;
	slap_ssf_t sss_update_ssf;
	slap_ssf_t sss_update_transport;
	slap_ssf_t sss_update_tls;
	slap_ssf_t sss_update_sasl;
	slap_ssf_t sss_simple_bind;
} slap_ssf_set_t;

/* Flags for telling slap_sasl_getdn() what type of identity is being passed */
#define SLAP_GETDN_AUTHCID 2
#define SLAP_GETDN_AUTHZID 4

/*
 * Index types
 */
#define SLAP_INDEX_TYPE           0x00FFUL
#define SLAP_INDEX_UNDEFINED      0x0001UL
#define SLAP_INDEX_PRESENT        0x0002UL
#define SLAP_INDEX_EQUALITY       0x0004UL
#define SLAP_INDEX_APPROX         0x0008UL
#define SLAP_INDEX_SUBSTR         0x0010UL
#define SLAP_INDEX_EXTENDED		  0x0020UL

#define SLAP_INDEX_DEFAULT        SLAP_INDEX_EQUALITY

#define IS_SLAP_INDEX(mask, type)	(((mask) & (type)) == (type))

#define SLAP_INDEX_SUBSTR_TYPE    0x0F00UL

#define SLAP_INDEX_SUBSTR_INITIAL ( SLAP_INDEX_SUBSTR | 0x0100UL ) 
#define SLAP_INDEX_SUBSTR_ANY     ( SLAP_INDEX_SUBSTR | 0x0200UL )
#define SLAP_INDEX_SUBSTR_FINAL   ( SLAP_INDEX_SUBSTR | 0x0400UL )
#define SLAP_INDEX_SUBSTR_DEFAULT \
	( SLAP_INDEX_SUBSTR \
	| SLAP_INDEX_SUBSTR_INITIAL \
	| SLAP_INDEX_SUBSTR_ANY \
	| SLAP_INDEX_SUBSTR_FINAL )

#define SLAP_INDEX_SUBSTR_MINLEN	2
#define SLAP_INDEX_SUBSTR_MAXLEN	4
#define SLAP_INDEX_SUBSTR_STEP	2

#define SLAP_INDEX_FLAGS         0xF000UL
#define SLAP_INDEX_NOSUBTYPES    0x1000UL /* don't use index w/ subtypes */
#define SLAP_INDEX_NOTAGS        0x2000UL /* don't use index w/ tags */

/*
 * there is a single index for each attribute.  these prefixes ensure
 * that there is no collision among keys.
 */
#define SLAP_INDEX_EQUALITY_PREFIX	'=' 	/* prefix for equality keys     */
#define SLAP_INDEX_APPROX_PREFIX	'~'		/* prefix for approx keys       */
#define SLAP_INDEX_SUBSTR_PREFIX	'*'		/* prefix for substring keys    */
#define SLAP_INDEX_SUBSTR_INITIAL_PREFIX '^'
#define SLAP_INDEX_SUBSTR_FINAL_PREFIX '$'
#define SLAP_INDEX_CONT_PREFIX		'.'		/* prefix for continuation keys */

#define SLAP_SYNTAX_MATCHINGRULES_OID	 "1.3.6.1.4.1.1466.115.121.1.30"
#define SLAP_SYNTAX_ATTRIBUTETYPES_OID	 "1.3.6.1.4.1.1466.115.121.1.3"
#define SLAP_SYNTAX_OBJECTCLASSES_OID	 "1.3.6.1.4.1.1466.115.121.1.37"
#define SLAP_SYNTAX_MATCHINGRULEUSES_OID "1.3.6.1.4.1.1466.115.121.1.31"
#define SLAP_SYNTAX_CONTENTRULE_OID		 "1.3.6.1.4.1.1466.115.121.1.16"

#ifdef LDAP_CLIENT_UPDATE
#define LCUP_COOKIE_OID "1.3.6.1.4.1.4203.666.10.1"
#endif /* LDAP_CLIENT_UPDATE */

/*
 * represents schema information for a database
 */
#define SLAP_SCHERR_OUTOFMEM			1
#define SLAP_SCHERR_CLASS_NOT_FOUND		2
#define SLAP_SCHERR_CLASS_BAD_USAGE		3
#define SLAP_SCHERR_CLASS_BAD_SUP		4
#define SLAP_SCHERR_CLASS_DUP			5
#define SLAP_SCHERR_ATTR_NOT_FOUND		6
#define SLAP_SCHERR_ATTR_BAD_MR			7
#define SLAP_SCHERR_ATTR_BAD_USAGE		8
#define SLAP_SCHERR_ATTR_BAD_SUP		9
#define SLAP_SCHERR_ATTR_INCOMPLETE		10
#define SLAP_SCHERR_ATTR_DUP			11
#define SLAP_SCHERR_MR_NOT_FOUND		12
#define SLAP_SCHERR_MR_INCOMPLETE		13
#define SLAP_SCHERR_MR_DUP				14
#define SLAP_SCHERR_SYN_NOT_FOUND		15
#define SLAP_SCHERR_SYN_DUP				16
#define SLAP_SCHERR_NO_NAME				17
#define SLAP_SCHERR_NOT_SUPPORTED		18
#define SLAP_SCHERR_BAD_DESCR			19
#define SLAP_SCHERR_OIDM				20
#define SLAP_SCHERR_CR_DUP				21
#define SLAP_SCHERR_CR_BAD_STRUCT		22
#define SLAP_SCHERR_CR_BAD_AUX			23
#define SLAP_SCHERR_CR_BAD_AT			24
#define SLAP_SCHERR_LAST				SLAP_SCHERR_CR_BAD_AT

typedef union slap_sockaddr {
	struct sockaddr sa_addr;
	struct sockaddr_in sa_in_addr;
#ifdef LDAP_PF_INET6
	struct sockaddr_storage sa_storage;
	struct sockaddr_in6 sa_in6_addr;
#endif
#ifdef LDAP_PF_LOCAL
	struct sockaddr_un sa_un_addr;
#endif
} Sockaddr;

#ifdef LDAP_PF_INET6
extern int slap_inet4or6;
#endif

typedef struct slap_oid_macro {
	struct berval som_oid;
	char **som_names;
	LDAP_SLIST_ENTRY(slap_oid_macro) som_next;
} OidMacro;

/* forward declarations */
struct slap_syntax;
struct slap_matching_rule;

typedef int slap_syntax_validate_func LDAP_P((
	struct slap_syntax *syntax,
	struct berval * in));

typedef int slap_syntax_transform_func LDAP_P((
	struct slap_syntax *syntax,
	struct berval * in,
	struct berval * out));

typedef struct slap_syntax {
	LDAPSyntax			ssyn_syn;
#define ssyn_oid		ssyn_syn.syn_oid
#define ssyn_desc		ssyn_syn.syn_desc
#define ssyn_extensions		ssyn_syn.syn_extensions
	/*
	 * Note: the former
	ber_len_t	ssyn_oidlen;
	 * has been replaced by a struct berval that uses the value
	 * provided by ssyn_syn.syn_oid; a macro that expands to
	 * the bv_len field of the berval is provided for backward
	 * compatibility.  CAUTION: NEVER FREE THE BERVAL
	 */
	struct berval	ssyn_bvoid;
#define	ssyn_oidlen	ssyn_bvoid.bv_len

	unsigned int ssyn_flags;

#define SLAP_SYNTAX_NONE	0x0000U
#define SLAP_SYNTAX_BLOB	0x0001U /* syntax treated as blob (audio) */
#define SLAP_SYNTAX_BINARY	0x0002U /* binary transfer required (certificate) */
#define SLAP_SYNTAX_BER		0x0004U /* stored in BER encoding (certificate) */
#define SLAP_SYNTAX_HIDE	0x8000U /* hide (do not publish) */

	slap_syntax_validate_func	*ssyn_validate;
	slap_syntax_transform_func	*ssyn_normalize;
	slap_syntax_transform_func	*ssyn_pretty;

#ifdef SLAPD_BINARY_CONVERSION
	/* convert to and from binary */
	slap_syntax_transform_func	*ssyn_ber2str;
	slap_syntax_transform_func	*ssyn_str2ber;
#endif

	LDAP_SLIST_ENTRY(slap_syntax) ssyn_next;
} Syntax;

#define slap_syntax_is_flag(s,flag) ((int)((s)->ssyn_flags & (flag)) ? 1 : 0)
#define slap_syntax_is_blob(s)		slap_syntax_is_flag((s),SLAP_SYNTAX_BLOB)
#define slap_syntax_is_binary(s)	slap_syntax_is_flag((s),SLAP_SYNTAX_BINARY)
#define slap_syntax_is_ber(s)		slap_syntax_is_flag((s),SLAP_SYNTAX_BER)
#define slap_syntax_is_hidden(s)	slap_syntax_is_flag((s),SLAP_SYNTAX_HIDE)

typedef struct slap_syntax_defs_rec {
	char *sd_desc;
	int sd_flags;
	slap_syntax_validate_func *sd_validate;
	slap_syntax_transform_func *sd_normalize;
	slap_syntax_transform_func *sd_pretty;
#ifdef SLAPD_BINARY_CONVERSION
	slap_syntax_transform_func *sd_ber2str;
	slap_syntax_transform_func *sd_str2ber;
#endif
} slap_syntax_defs_rec;

/* X -> Y Converter */
typedef int slap_mr_convert_func LDAP_P((
	struct berval * in,
	struct berval * out ));

/* Normalizer */
typedef int slap_mr_normalize_func LDAP_P((
	slap_mask_t use,
	struct slap_syntax *syntax, /* NULL if in is asserted value */
	struct slap_matching_rule *mr,
	struct berval * in,
	struct berval * out ));

/* Match (compare) function */
typedef int slap_mr_match_func LDAP_P((
	int *match,
	slap_mask_t use,
	struct slap_syntax *syntax,	/* syntax of stored value */
	struct slap_matching_rule *mr,
	struct berval * value,
	void * assertValue ));

/* Index generation function */
typedef int slap_mr_indexer_func LDAP_P((
	slap_mask_t use,
	slap_mask_t mask,
	struct slap_syntax *syntax,	/* syntax of stored value */
	struct slap_matching_rule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keys ));

/* Filter index function */
typedef int slap_mr_filter_func LDAP_P((
	slap_mask_t use,
	slap_mask_t mask,
	struct slap_syntax *syntax,	/* syntax of stored value */
	struct slap_matching_rule *mr,
	struct berval *prefix,
	void * assertValue,
	BerVarray *keys ));

typedef struct slap_matching_rule_use MatchingRuleUse;

typedef struct slap_matching_rule {
	LDAPMatchingRule		smr_mrule;
	MatchingRuleUse			*smr_mru;
	/* RFC2252 string representation */
	struct berval			smr_str;
	/*
	 * Note: the former
	 *			ber_len_t	smr_oidlen;
	 * has been replaced by a struct berval that uses the value
	 * provided by smr_mrule.mr_oid; a macro that expands to
	 * the bv_len field of the berval is provided for backward
	 * compatibility.  CAUTION: NEVER FREE THE BERVAL
	 */
	struct berval			smr_bvoid;
#define	smr_oidlen			smr_bvoid.bv_len

	slap_mask_t				smr_usage;

#define SLAP_MR_HIDE			0x8000U

#define SLAP_MR_TYPE_MASK		0x0F00U
#define SLAP_MR_SUBTYPE_MASK	0x00F0U
#define SLAP_MR_USAGE			0x000FU

#define SLAP_MR_NONE			0x0000U
#define SLAP_MR_EQUALITY		0x0100U
#define SLAP_MR_ORDERING		0x0200U
#define SLAP_MR_SUBSTR			0x0400U
#define SLAP_MR_EXT				0x0800U /* implicitly extensible */

#define SLAP_MR_EQUALITY_APPROX	( SLAP_MR_EQUALITY | 0x0010U )
#define SLAP_MR_DN_FOLD			0x0008U

#define SLAP_MR_SUBSTR_INITIAL	( SLAP_MR_SUBSTR | 0x0010U )
#define SLAP_MR_SUBSTR_ANY		( SLAP_MR_SUBSTR | 0x0020U )
#define SLAP_MR_SUBSTR_FINAL	( SLAP_MR_SUBSTR | 0x0040U )

/*
 * normally the provided value is expected to conform to
 * assertion syntax specified in the matching rule, however
 * at times (such as during individual value modification),
 * the provided value is expected to conform to the
 * attribute's value syntax.
 */
#define SLAP_MR_ASSERTION_SYNTAX_MATCH			0x0000U
#define SLAP_MR_VALUE_SYNTAX_MATCH				0x0001U
#define SLAP_MR_VALUE_SYNTAX_CONVERTED_MATCH	0x0003U
#define SLAP_MR_VALUE_NORMALIZED_MATCH	0x0004U

#define SLAP_IS_MR_ASSERTION_SYNTAX_MATCH( usage ) \
	(!((usage) & SLAP_MR_VALUE_SYNTAX_MATCH))
#define SLAP_IS_MR_VALUE_SYNTAX_MATCH( usage ) \
	((usage) & SLAP_MR_VALUE_SYNTAX_MATCH)

#define SLAP_IS_MR_VALUE_SYNTAX_CONVERTED_MATCH( usage ) \
	(((usage) & SLAP_MR_VALUE_SYNTAX_CONVERTED_MATCH) \
		== SLAP_MR_VALUE_SYNTAX_CONVERTED_MATCH)
#define SLAP_IS_MR_VALUE_SYNTAX_NONCONVERTED_MATCH( usage ) \
	(((usage) & SLAP_MR_VALUE_SYNTAX_CONVERTED_MATCH) \
		== SLAP_MR_VALUE_SYNTAX_MATCH)

	Syntax					*smr_syntax;
	slap_mr_convert_func	*smr_convert;
	slap_mr_normalize_func	*smr_normalize;
	slap_mr_match_func		*smr_match;
	slap_mr_indexer_func	*smr_indexer;
	slap_mr_filter_func		*smr_filter;

	/*
	 * null terminated array of syntaxes compatible with this syntax
	 * note: when MS_EXT is set, this MUST NOT contain the assertion
	 * syntax of the rule.  When MS_EXT is not set, it MAY.
	 */
	Syntax					**smr_compat_syntaxes;

	struct slap_matching_rule	*smr_associated;
	LDAP_SLIST_ENTRY(slap_matching_rule)smr_next;

#define smr_oid				smr_mrule.mr_oid
#define smr_names			smr_mrule.mr_names
#define smr_desc			smr_mrule.mr_desc
#define smr_obsolete		smr_mrule.mr_obsolete
#define smr_syntax_oid		smr_mrule.mr_syntax_oid
#define smr_extensions		smr_mrule.mr_extensions
} MatchingRule;

struct slap_matching_rule_use {
	LDAPMatchingRuleUse		smru_mruleuse;
	MatchingRule			*smru_mr;
	/* RFC2252 string representation */
	struct berval			smru_str;

	LDAP_SLIST_ENTRY(slap_matching_rule_use) smru_next;

#define smru_oid			smru_mruleuse.mru_oid
#define smru_names			smru_mruleuse.mru_names
#define smru_desc			smru_mruleuse.mru_desc
#define smru_obsolete			smru_mruleuse.mru_obsolete
#define smru_applies_oids		smru_mruleuse.mru_applies_oids

#define smru_usage			smru_mr->smr_usage
} /* MatchingRuleUse */ ;

typedef struct slap_mrule_defs_rec {
	char *						mrd_desc;
	slap_mask_t					mrd_usage;
	char **						mrd_compat_syntaxes;
	slap_mr_convert_func *		mrd_convert;
	slap_mr_normalize_func *	mrd_normalize;
	slap_mr_match_func *		mrd_match;
	slap_mr_indexer_func *		mrd_indexer;
	slap_mr_filter_func *		mrd_filter;

	char *						mrd_associated;
} slap_mrule_defs_rec;

struct slap_backend_db;
struct slap_entry;
struct slap_attr;

typedef int (AttributeTypeSchemaCheckFN)(
	struct slap_backend_db *be,
	struct slap_entry *e,
	struct slap_attr *attr,
	const char** text,
	char *textbuf, size_t textlen );

typedef struct slap_attribute_type {
	LDAPAttributeType		sat_atype;
	struct berval			sat_cname;
	struct slap_attribute_type	*sat_sup;
	struct slap_attribute_type	**sat_subtypes;
	MatchingRule			*sat_equality;
	MatchingRule			*sat_approx;
	MatchingRule			*sat_ordering;
	MatchingRule			*sat_substr;
	Syntax					*sat_syntax;

	AttributeTypeSchemaCheckFN	*sat_check;

#define SLAP_AT_NONE		0x0000U
#define SLAP_AT_ABSTRACT	0x0100U /* cannot be instantiated */
#define SLAP_AT_FINAL		0x0200U /* cannot be subtyped */
#define SLAP_AT_HIDE		0x8000U /* hide attribute */
	slap_mask_t					sat_flags;

	LDAP_SLIST_ENTRY(slap_attribute_type) sat_next;

#define sat_oid				sat_atype.at_oid
#define sat_names			sat_atype.at_names
#define sat_desc			sat_atype.at_desc
#define sat_obsolete		sat_atype.at_obsolete
#define sat_sup_oid			sat_atype.at_sup_oid
#define sat_equality_oid	sat_atype.at_equality_oid
#define sat_ordering_oid	sat_atype.at_ordering_oid
#define sat_substr_oid		sat_atype.at_substr_oid
#define sat_syntax_oid		sat_atype.at_syntax_oid
#define sat_single_value	sat_atype.at_single_value
#define sat_collective		sat_atype.at_collective
#define sat_no_user_mod		sat_atype.at_no_user_mod
#define sat_usage			sat_atype.at_usage
#define sat_extensions		sat_atype.at_extensions

	struct slap_attr_desc		*sat_ad;
	ldap_pvt_thread_mutex_t		sat_ad_mutex;
} AttributeType;

#define is_at_operational(at)	((at)->sat_usage)
#define is_at_single_value(at)	((at)->sat_single_value)
#define is_at_collective(at)	((at)->sat_collective)
#define is_at_obsolete(at)		((at)->sat_obsolete)
#define is_at_no_user_mod(at)	((at)->sat_no_user_mod)

struct slap_object_class;

typedef int (ObjectClassSchemaCheckFN)(
	struct slap_backend_db *be,
	struct slap_entry *e,
	struct slap_object_class *oc,
	const char** text,
	char *textbuf, size_t textlen );

typedef struct slap_object_class {
	LDAPObjectClass			soc_oclass;
	struct berval			soc_cname;
	struct slap_object_class	**soc_sups;
	AttributeType				**soc_required;
	AttributeType				**soc_allowed;
	ObjectClassSchemaCheckFN	*soc_check;
	slap_mask_t					soc_flags;
#define soc_oid				soc_oclass.oc_oid
#define soc_names			soc_oclass.oc_names
#define soc_desc			soc_oclass.oc_desc
#define soc_obsolete		soc_oclass.oc_obsolete
#define soc_sup_oids		soc_oclass.oc_sup_oids
#define soc_kind			soc_oclass.oc_kind
#define soc_at_oids_must	soc_oclass.oc_at_oids_must
#define soc_at_oids_may		soc_oclass.oc_at_oids_may
#define soc_extensions		soc_oclass.oc_extensions

	LDAP_SLIST_ENTRY(slap_object_class) soc_next;
} ObjectClass;

#define	SLAP_OC_ALIAS		0x0001
#define	SLAP_OC_REFERRAL	0x0002
#define	SLAP_OC_SUBENTRY	0x0004
#define	SLAP_OC_DYNAMICOBJECT	0x0008
#define	SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY	0x0010
#define	SLAP_OC__MASK		0x001F
#define	SLAP_OC__END		0x0020
#define SLAP_OC_OPERATIONAL	0x4000
#define SLAP_OC_HIDE		0x8000

/*
 * DIT content rule
 */
typedef struct slap_content_rule {
	LDAPContentRule		scr_crule;
	ObjectClass			*scr_sclass;
	ObjectClass			**scr_auxiliaries;	/* optional */
	AttributeType		**scr_required;		/* optional */
	AttributeType		**scr_allowed;		/* optional */
	AttributeType		**scr_precluded;	/* optional */
#define scr_oid				scr_crule.cr_oid
#define scr_names			scr_crule.cr_names
#define scr_desc			scr_crule.cr_desc
#define scr_obsolete		scr_crule.cr_obsolete
#define scr_oc_oids_aux		scr_crule.cr_oc_oids_aux
#define scr_at_oids_must	scr_crule.cr_at_oids_must
#define scr_at_oids_may		scr_crule.cr_at_oids_may
#define scr_at_oids_not		scr_crule.cr_at_oids_not

	LDAP_SLIST_ENTRY( slap_content_rule ) scr_next;
} ContentRule;

/* Represents a recognized attribute description ( type + options ). */
typedef struct slap_attr_desc {
	struct slap_attr_desc *ad_next;
	AttributeType *ad_type;		/* attribute type, must be specified */
	struct berval ad_cname;		/* canonical name, must be specified */
	struct berval ad_tags;		/* empty if no tagging options */
	unsigned ad_flags;
#define SLAP_DESC_NONE			0x00U
#define SLAP_DESC_BINARY		0x01U
#define SLAP_DESC_TAG_RANGE		0x80U
} AttributeDescription;

typedef struct slap_attr_name {
	struct berval an_name;
	AttributeDescription *an_desc;
	ObjectClass *an_oc;
} AttributeName;

#define slap_ad_is_tagged(ad)			( (ad)->ad_tags.bv_len != 0 )
#define slap_ad_is_tag_range(ad)	\
	( ((ad)->ad_flags & SLAP_DESC_TAG_RANGE) ? 1 : 0 )
#define slap_ad_is_binary(ad)		\
	( ((ad)->ad_flags & SLAP_DESC_BINARY) ? 1 : 0 )

/*
 * pointers to schema elements used internally
 */
struct slap_internal_schema {
	/* objectClass */
	ObjectClass *si_oc_top;
	ObjectClass *si_oc_extensibleObject;
	ObjectClass *si_oc_alias;
	ObjectClass *si_oc_referral;
	ObjectClass *si_oc_rootdse;
	ObjectClass *si_oc_subentry;
	ObjectClass *si_oc_subschema;
	ObjectClass *si_oc_monitor;
	ObjectClass *si_oc_collectiveAttributeSubentry;
	ObjectClass *si_oc_dynamicObject;

	/* objectClass attribute descriptions */
	AttributeDescription *si_ad_objectClass;

	/* operational attribute descriptions */
	AttributeDescription *si_ad_structuralObjectClass;
	AttributeDescription *si_ad_creatorsName;
	AttributeDescription *si_ad_createTimestamp;
	AttributeDescription *si_ad_modifiersName;
	AttributeDescription *si_ad_modifyTimestamp;
	AttributeDescription *si_ad_hasSubordinates;
	AttributeDescription *si_ad_subschemaSubentry;
	AttributeDescription *si_ad_collectiveSubentries;
	AttributeDescription *si_ad_collectiveExclusions;
	AttributeDescription *si_ad_entryUUID;
	AttributeDescription *si_ad_entryCSN;
	AttributeDescription *si_ad_superiorUUID;

	/* root DSE attribute descriptions */
	AttributeDescription *si_ad_altServer;
	AttributeDescription *si_ad_namingContexts;
	AttributeDescription *si_ad_supportedControl;
	AttributeDescription *si_ad_supportedExtension;
	AttributeDescription *si_ad_supportedLDAPVersion;
	AttributeDescription *si_ad_supportedSASLMechanisms;
	AttributeDescription *si_ad_supportedFeatures;
	AttributeDescription *si_ad_monitorContext;
	AttributeDescription *si_ad_vendorName;
	AttributeDescription *si_ad_vendorVersion;

	/* subentry attribute descriptions */
	AttributeDescription *si_ad_administrativeRole;
	AttributeDescription *si_ad_subtreeSpecification;

	/* subschema subentry attribute descriptions */
	AttributeDescription *si_ad_ditStructureRules;
	AttributeDescription *si_ad_ditContentRules;
	AttributeDescription *si_ad_nameForms;
	AttributeDescription *si_ad_objectClasses;
	AttributeDescription *si_ad_attributeTypes;
	AttributeDescription *si_ad_ldapSyntaxes;
	AttributeDescription *si_ad_matchingRules;
	AttributeDescription *si_ad_matchingRuleUse;

	/* Aliases & Referrals */
	AttributeDescription *si_ad_aliasedObjectName;
	AttributeDescription *si_ad_ref;

	/* Access Control Internals */
	AttributeDescription *si_ad_entry;
	AttributeDescription *si_ad_children;
	AttributeDescription *si_ad_saslAuthzTo;
	AttributeDescription *si_ad_saslAuthzFrom;
#ifdef SLAPD_ACI_ENABLED
	AttributeDescription *si_ad_aci;
#endif

	/* dynamic entries */
	AttributeDescription *si_ad_entryTtl;
	AttributeDescription *si_ad_dynamicSubtrees;

	/* Other attributes descriptions */
	AttributeDescription *si_ad_distinguishedName;
	AttributeDescription *si_ad_name;
	AttributeDescription *si_ad_cn;
	AttributeDescription *si_ad_userPassword;
#ifdef SLAPD_AUTHPASSWD
	AttributeDescription *si_ad_authPassword;
#endif
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
	AttributeDescription *si_ad_krbName;
#endif

	/* Undefined Attribute Type */
	AttributeType	*si_at_undefined;

	/* Matching Rules */
	MatchingRule	*si_mr_distinguishedNameMatch;
	MatchingRule	*si_mr_integerMatch;

	/* Syntaxes */
	Syntax		*si_syn_octetString;
	Syntax		*si_syn_distinguishedName;
	Syntax		*si_syn_integer;
};

typedef struct slap_attr_assertion {
	AttributeDescription	*aa_desc;
	struct berval aa_value;
} AttributeAssertion;

typedef struct slap_ss_assertion {
	AttributeDescription	*sa_desc;
	struct berval		sa_initial;
	struct berval		*sa_any;
	struct berval		sa_final;
} SubstringsAssertion;

typedef struct slap_mr_assertion {
	MatchingRule		*ma_rule;	/* optional */
	struct berval		ma_rule_text;  /* optional */
	AttributeDescription	*ma_desc;	/* optional */
	int						ma_dnattrs; /* boolean */
	struct berval		ma_value;	/* required */
} MatchingRuleAssertion;

/*
 * represents a search filter
 */
typedef struct slap_filter {
	ber_tag_t	f_choice;	/* values taken from ldap.h, plus: */
#define SLAPD_FILTER_COMPUTED	((ber_tag_t) -1)
#define SLAPD_FILTER_DN_ONE		((ber_tag_t) -2)
#define SLAPD_FILTER_DN_SUBTREE	((ber_tag_t) -3)

	union f_un_u {
		/* precomputed result */
		ber_int_t f_un_result;

		/* DN */
		struct berval *f_un_dn;

		/* present */
		AttributeDescription *f_un_desc;

		/* simple value assertion */
		AttributeAssertion *f_un_ava;

		/* substring assertion */
		SubstringsAssertion *f_un_ssa;

		/* matching rule assertion */
		MatchingRuleAssertion *f_un_mra;

#define f_dn			f_un.f_un_dn
#define f_desc			f_un.f_un_desc
#define f_ava			f_un.f_un_ava
#define f_av_desc		f_un.f_un_ava->aa_desc
#define f_av_value		f_un.f_un_ava->aa_value
#define f_sub			f_un.f_un_ssa
#define f_sub_desc		f_un.f_un_ssa->sa_desc
#define f_sub_initial	f_un.f_un_ssa->sa_initial
#define f_sub_any		f_un.f_un_ssa->sa_any
#define f_sub_final		f_un.f_un_ssa->sa_final
#define f_mra			f_un.f_un_mra
#define f_mr_rule		f_un.f_un_mra->ma_rule
#define f_mr_rule_text	f_un.f_un_mra->ma_rule_text
#define f_mr_desc		f_un.f_un_mra->ma_desc
#define f_mr_value		f_un.f_un_mra->ma_value
#define	f_mr_dnattrs	f_un.f_un_mra->ma_dnattrs

		/* and, or, not */
		struct slap_filter *f_un_complex;
	} f_un;

#define f_result	f_un.f_un_result
#define f_and		f_un.f_un_complex
#define f_or		f_un.f_un_complex
#define f_not		f_un.f_un_complex
#define f_list		f_un.f_un_complex

	struct slap_filter	*f_next;
} Filter;

/* compare routines can return undefined */
#define SLAPD_COMPARE_UNDEFINED	((ber_int_t) -1)

typedef struct slap_valuesreturnfilter {
	ber_tag_t	vrf_choice;

	union vrf_un_u {
		/* precomputed result */
		ber_int_t vrf_un_result;

		/* DN */
		char *vrf_un_dn;

		/* present */
		AttributeDescription *vrf_un_desc;

		/* simple value assertion */
		AttributeAssertion *vrf_un_ava;

		/* substring assertion */
		SubstringsAssertion *vrf_un_ssa;

		/* matching rule assertion */
		MatchingRuleAssertion *vrf_un_mra;

#define vrf_result		vrf_un.vrf_un_result
#define vrf_dn			vrf_un.vrf_un_dn
#define vrf_desc		vrf_un.vrf_un_desc
#define vrf_ava			vrf_un.vrf_un_ava
#define vrf_av_desc		vrf_un.vrf_un_ava->aa_desc
#define vrf_av_value	vrf_un.vrf_un_ava->aa_value
#define vrf_ssa			vrf_un.vrf_un_ssa
#define vrf_sub			vrf_un.vrf_un_ssa
#define vrf_sub_desc	vrf_un.vrf_un_ssa->sa_desc
#define vrf_sub_initial	vrf_un.vrf_un_ssa->sa_initial
#define vrf_sub_any		vrf_un.vrf_un_ssa->sa_any
#define vrf_sub_final	vrf_un.vrf_un_ssa->sa_final
#define vrf_mra			vrf_un.vrf_un_mra
#define vrf_mr_rule		vrf_un.vrf_un_mra->ma_rule
#define vrf_mr_rule_text	vrf_un.vrf_un_mra->ma_rule_text
#define vrf_mr_desc		vrf_un.vrf_un_mra->ma_desc
#define vrf_mr_value		vrf_un.vrf_un_mra->ma_value
#define	vrf_mr_dnattrs	vrf_un.vrf_un_mra->ma_dnattrs


	} vrf_un;

	struct slap_valuesreturnfilter	*vrf_next;
} ValuesReturnFilter;

/*
 * represents an attribute (description + values)
 */
typedef struct slap_attr {
	AttributeDescription *a_desc;
	BerVarray	a_vals;
	struct slap_attr	*a_next;
	unsigned a_flags;
#define SLAP_ATTR_IXADD		0x1U
#define SLAP_ATTR_IXDEL		0x2U
} Attribute;


/*
 * the id used in the indexes to refer to an entry
 */
typedef unsigned long	ID;
#define NOID	((ID)~0)

/*
 * represents an entry in core
 */
typedef struct slap_entry {
	/*
	 * The ID field should only be changed before entry is
	 * inserted into a cache.  The ID value is backend
	 * specific.
	 */
	ID		e_id;

	struct berval e_name;	/* name (DN) of this entry */
	struct berval e_nname;	/* normalized name (DN) of this entry */

	/* for migration purposes */
#define e_dn e_name.bv_val
#define e_ndn e_nname.bv_val

	Attribute	*e_attrs;	/* list of attributes + values */

	slap_mask_t	e_ocflags;

	struct berval	e_bv;		/* For entry_encode/entry_decode */

	/* for use by the backend for any purpose */
	void*	e_private;
} Entry;

/*
 * A list of LDAPMods
 */
typedef struct slap_mod {
	int sm_op;
	AttributeDescription *sm_desc;
	struct berval sm_type;
	BerVarray sm_bvalues;
} Modification;

typedef struct slap_mod_list {
	Modification sml_mod;
#define sml_op		sml_mod.sm_op
#define sml_desc	sml_mod.sm_desc
#define	sml_type	sml_mod.sm_type
#define sml_bvalues	sml_mod.sm_bvalues
	struct slap_mod_list *sml_next;
} Modifications;

typedef struct slap_ldap_modlist {
	LDAPMod ml_mod;
	struct slap_ldap_modlist *ml_next;
#define ml_op		ml_mod.mod_op
#define ml_type		ml_mod.mod_type
#define ml_values	ml_mod.mod_values
#define ml_bvalues	ml_mod.mod_bvalues
} LDAPModList;

/*
 * represents an access control list
 */
typedef enum slap_access_e {
	ACL_INVALID_ACCESS = -1,
	ACL_NONE = 0,
	ACL_AUTH,
	ACL_COMPARE,
	ACL_SEARCH,
	ACL_READ,
	ACL_WRITE
} slap_access_t;

typedef enum slap_control_e {
	ACL_INVALID_CONTROL	= 0,
	ACL_STOP,
	ACL_CONTINUE,
	ACL_BREAK
} slap_control_t;

typedef enum slap_style_e {
	ACL_STYLE_REGEX = 0,
	ACL_STYLE_BASE,
	ACL_STYLE_ONE,
	ACL_STYLE_SUBTREE,
	ACL_STYLE_CHILDREN,
	ACL_STYLE_ATTROF,

	/* alternate names */
	ACL_STYLE_EXACT = ACL_STYLE_BASE
} slap_style_t;

typedef struct slap_authz_info {
	ber_tag_t	sai_method;		/* LDAP_AUTH_* from <ldap.h> */
	struct berval	sai_mech;		/* SASL Mechanism */
	struct berval	sai_dn;			/* DN for reporting purposes */
	struct berval	sai_ndn;		/* Normalized DN */

	/* Security Strength Factors */
	slap_ssf_t	sai_ssf;			/* Overall SSF */
	slap_ssf_t	sai_transport_ssf;	/* Transport SSF */
	slap_ssf_t	sai_tls_ssf;		/* TLS SSF */
	slap_ssf_t	sai_sasl_ssf;		/* SASL SSF */
} AuthorizationInformation;

/* the "by" part */
typedef struct slap_access {
	slap_control_t a_type;

#define ACL_ACCESS2PRIV(access)	(0x01U << (access))

#define ACL_PRIV_NONE			ACL_ACCESS2PRIV( ACL_NONE )
#define ACL_PRIV_AUTH			ACL_ACCESS2PRIV( ACL_AUTH )
#define ACL_PRIV_COMPARE		ACL_ACCESS2PRIV( ACL_COMPARE )
#define ACL_PRIV_SEARCH			ACL_ACCESS2PRIV( ACL_SEARCH )
#define ACL_PRIV_READ			ACL_ACCESS2PRIV( ACL_READ )
#define ACL_PRIV_WRITE			ACL_ACCESS2PRIV( ACL_WRITE )

#define ACL_PRIV_MASK			0x00ffUL

/* priv flags */
#define ACL_PRIV_LEVEL			0x1000UL
#define ACL_PRIV_ADDITIVE		0x2000UL
#define ACL_PRIV_SUBSTRACTIVE	0x4000UL

/* invalid privs */
#define ACL_PRIV_INVALID		0x0UL

#define ACL_PRIV_ISSET(m,p)		(((m) & (p)) == (p))
#define ACL_PRIV_ASSIGN(m,p)	do { (m)  =  (p); } while(0)
#define ACL_PRIV_SET(m,p)		do { (m) |=  (p); } while(0)
#define ACL_PRIV_CLR(m,p)		do { (m) &= ~(p); } while(0)

#define ACL_INIT(m)				ACL_PRIV_ASSIGN(m, ACL_PRIV_NONE)
#define ACL_INVALIDATE(m)		ACL_PRIV_ASSIGN(m, ACL_PRIV_INVALID)

#define ACL_GRANT(m,a)			ACL_PRIV_ISSET((m),ACL_ACCESS2PRIV(a))

#define ACL_IS_INVALID(m)		((m) == ACL_PRIV_INVALID)

#define ACL_IS_LEVEL(m)			ACL_PRIV_ISSET((m),ACL_PRIV_LEVEL)
#define ACL_IS_ADDITIVE(m)		ACL_PRIV_ISSET((m),ACL_PRIV_ADDITIVE)
#define ACL_IS_SUBTRACTIVE(m)	ACL_PRIV_ISSET((m),ACL_PRIV_SUBSTRACTIVE)

#define ACL_LVL_NONE			(ACL_PRIV_NONE|ACL_PRIV_LEVEL)
#define ACL_LVL_AUTH			(ACL_PRIV_AUTH|ACL_LVL_NONE)
#define ACL_LVL_COMPARE			(ACL_PRIV_COMPARE|ACL_LVL_AUTH)
#define ACL_LVL_SEARCH			(ACL_PRIV_SEARCH|ACL_LVL_COMPARE)
#define ACL_LVL_READ			(ACL_PRIV_READ|ACL_LVL_SEARCH)
#define ACL_LVL_WRITE			(ACL_PRIV_WRITE|ACL_LVL_READ)

#define ACL_LVL(m,l)			(((m)&ACL_PRIV_MASK) == ((l)&ACL_PRIV_MASK))
#define ACL_LVL_IS_NONE(m)		ACL_LVL((m),ACL_LVL_NONE)
#define ACL_LVL_IS_AUTH(m)		ACL_LVL((m),ACL_LVL_AUTH)
#define ACL_LVL_IS_COMPARE(m)	ACL_LVL((m),ACL_LVL_COMPARE)
#define ACL_LVL_IS_SEARCH(m)	ACL_LVL((m),ACL_LVL_SEARCH)
#define ACL_LVL_IS_READ(m)		ACL_LVL((m),ACL_LVL_READ)
#define ACL_LVL_IS_WRITE(m)		ACL_LVL((m),ACL_LVL_WRITE)

#define ACL_LVL_ASSIGN_NONE(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_NONE)
#define ACL_LVL_ASSIGN_AUTH(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_AUTH)
#define ACL_LVL_ASSIGN_COMPARE(m)	ACL_PRIV_ASSIGN((m),ACL_LVL_COMPARE)
#define ACL_LVL_ASSIGN_SEARCH(m)	ACL_PRIV_ASSIGN((m),ACL_LVL_SEARCH)
#define ACL_LVL_ASSIGN_READ(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_READ)
#define ACL_LVL_ASSIGN_WRITE(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_WRITE)

	slap_mask_t	a_access_mask;

	AuthorizationInformation	a_authz;
#define a_dn_pat	a_authz.sai_dn

	slap_style_t a_dn_style;
	AttributeDescription	*a_dn_at;
	int			a_dn_self;
	int 			a_dn_expand;

	slap_style_t a_peername_style;
	struct berval	a_peername_pat;
	slap_style_t a_sockname_style;
	struct berval	a_sockname_pat;

	slap_style_t a_domain_style;
	struct berval	a_domain_pat;
	int		a_domain_expand;

	slap_style_t a_sockurl_style;
	struct berval	a_sockurl_pat;
	slap_style_t a_set_style;
	struct berval	a_set_pat;

#ifdef SLAPD_ACI_ENABLED
	AttributeDescription	*a_aci_at;
#endif

	/* ACL Groups */
	slap_style_t a_group_style;
	struct berval	a_group_pat;
	ObjectClass				*a_group_oc;
	AttributeDescription	*a_group_at;

	struct slap_access	*a_next;
} Access;

/* the "to" part */
typedef struct slap_acl {
	/* "to" part: the entries this acl applies to */
	Filter		*acl_filter;
	slap_style_t acl_dn_style;
	regex_t		acl_dn_re;
	struct berval	acl_dn_pat;
	AttributeName	*acl_attrs;

	/* "by" part: list of who has what access to the entries */
	Access	*acl_access;

	struct slap_acl	*acl_next;
} AccessControl;

typedef struct slap_acl_state {
	unsigned as_recorded;
#define ACL_STATE_NOT_RECORDED			0x0
#define ACL_STATE_RECORDED_VD			0x1
#define ACL_STATE_RECORDED_NV			0x2
#define ACL_STATE_RECORDED				0x3

	/* Access state */
	AccessControl *as_vd_acl;
	AccessControl *as_vi_acl;
	slap_mask_t as_vd_acl_mask;
	regmatch_t as_vd_acl_matches[MAXREMATCHES];
	int as_vd_acl_count;

	Access *as_vd_access;
	int as_vd_access_count;

	int as_result;
	AttributeDescription *as_vd_ad;
} AccessControlState;
#define ACL_STATE_INIT { ACL_STATE_NOT_RECORDED, NULL, NULL, 0UL, \
	{ { 0, 0 } }, 0, NULL, 0, 0, NULL }

/*
 * replog moddn param structure
 */
struct slap_replog_moddn {
	struct berval *newrdn;
	int	deloldrdn;
	struct berval *newsup;
};

/*
 * Backend-info
 * represents a backend 
 */

typedef struct slap_backend_info BackendInfo;	/* per backend type */
typedef struct slap_backend_db BackendDB;		/* per backend database */

LDAP_SLAPD_V (int) nBackendInfo;
LDAP_SLAPD_V (int) nBackendDB;
LDAP_SLAPD_V (BackendInfo *) backendInfo;
LDAP_SLAPD_V (BackendDB *) backendDB;

LDAP_SLAPD_V (int) slapMode;	
#define SLAP_UNDEFINED_MODE	0x0000
#define SLAP_SERVER_MODE	0x0001
#define SLAP_TOOL_MODE		0x0002
#define SLAP_MODE			0x0003

#define SLAP_TRUNCATE_MODE	0x0100

struct slap_replica_info {
	char *ri_host;				/* supersedes be_replica */
	BerVarray ri_nsuffix;	/* array of suffixes this replica accepts */
	AttributeName *ri_attrs;	/* attrs to replicate, NULL=all */
	int ri_exclude;			/* 1 => exclude ri_attrs */
};

struct slap_limits_set {
	/* time limits */
	int	lms_t_soft;
	int	lms_t_hard;

	/* size limits */
	int	lms_s_soft;
	int	lms_s_hard;
	int	lms_s_unchecked;
	int	lms_s_pr;
	int	lms_s_pr_hide;
};

struct slap_limits {
	int     lm_type;	/* type of pattern */
#define SLAP_LIMITS_UNDEFINED	0x0000
#define SLAP_LIMITS_EXACT	0x0001
#define SLAP_LIMITS_BASE	SLAP_LIMITS_EXACT
#define SLAP_LIMITS_ONE		0x0002
#define SLAP_LIMITS_SUBTREE	0x0003
#define SLAP_LIMITS_CHILDREN	0x0004
#define SLAP_LIMITS_REGEX	0x0005
#define SLAP_LIMITS_ANONYMOUS	0x0006
#define SLAP_LIMITS_USERS	0x0007
#define SLAP_LIMITS_ANY		0x0008
	regex_t	lm_dn_regex;		/* regex data for REGEX */

	/*
	 * normalized DN for EXACT, BASE, ONE, SUBTREE, CHILDREN;
	 * pattern for REGEX; NULL for ANONYMOUS, USERS
	 */
	struct berval lm_dn_pat;

	struct slap_limits_set	lm_limits;
};

/* temporary aliases */
typedef BackendDB Backend;
#define nbackends nBackendDB
#define backends backendDB

struct slap_backend_db {
	BackendInfo	*bd_info;	/* pointer to shared backend info */

	/* BackendInfo accessors */
#define		be_config	bd_info->bi_db_config
#define		be_type		bd_info->bi_type

#define		be_bind		bd_info->bi_op_bind
#define		be_unbind	bd_info->bi_op_unbind
#define		be_add		bd_info->bi_op_add
#define		be_compare	bd_info->bi_op_compare
#define		be_delete	bd_info->bi_op_delete
#define		be_modify	bd_info->bi_op_modify
#define		be_modrdn	bd_info->bi_op_modrdn
#define		be_search	bd_info->bi_op_search
#define		be_abandon	bd_info->bi_op_abandon
#define		be_cancel	bd_info->bi_op_cancel

#define		be_extended	bd_info->bi_extended

#define		be_release	bd_info->bi_entry_release_rw
#define		be_chk_referrals	bd_info->bi_chk_referrals
#define		be_group	bd_info->bi_acl_group
#define		be_attribute	bd_info->bi_acl_attribute
#define		be_operational	bd_info->bi_operational

/*
 * define to honor hasSubordinates operational attribute in search filters
 * (in previous use there was a flaw with back-bdb and back-ldbm; now it 
 * is fixed).
 */
#define		be_has_subordinates bd_info->bi_has_subordinates

#define		be_controls	bd_info->bi_controls

#define		be_connection_init	bd_info->bi_connection_init
#define		be_connection_destroy	bd_info->bi_connection_destroy

#ifdef SLAPD_TOOLS
#define		be_entry_open bd_info->bi_tool_entry_open
#define		be_entry_close bd_info->bi_tool_entry_close
#define		be_entry_first bd_info->bi_tool_entry_first
#define		be_entry_next bd_info->bi_tool_entry_next
#define		be_entry_reindex bd_info->bi_tool_entry_reindex
#define		be_entry_get bd_info->bi_tool_entry_get
#define		be_entry_put bd_info->bi_tool_entry_put
#define		be_sync bd_info->bi_tool_sync
#endif

#define SLAP_BFLAG_NOLASTMOD		0x0001U
#define	SLAP_BFLAG_GLUE_INSTANCE	0x0010U	/* a glue backend */
#define	SLAP_BFLAG_GLUE_SUBORDINATE	0x0020U	/* child of a glue hierarchy */
#define	SLAP_BFLAG_GLUE_LINKED		0x0040U	/* child is connected to parent */
#define SLAP_BFLAG_ALIASES		0x0100U
#define SLAP_BFLAG_REFERRALS	0x0200U
#define SLAP_BFLAG_SUBENTRIES	0x0400U
#define SLAP_BFLAG_MONITOR		0x1000U
#define SLAP_BFLAG_DYNAMIC		0x2000U
	slap_mask_t	be_flags;
#define SLAP_LASTMOD(be)	(!((be)->be_flags & SLAP_BFLAG_NOLASTMOD))
#define	SLAP_GLUE_INSTANCE(be)	((be)->be_flags & SLAP_BFLAG_GLUE_INSTANCE)
#define	SLAP_GLUE_SUBORDINATE(be) \
	((be)->be_flags & SLAP_BFLAG_GLUE_SUBORDINATE)
#define	SLAP_GLUE_LINKED(be)	((be)->be_flags & SLAP_BFLAG_GLUE_LINKED)
#define SLAP_ALIASES(be)	((be)->be_flags & SLAP_BFLAG_ALIASES)
#define SLAP_REFERRALS(be)	((be)->be_flags & SLAP_BFLAG_REFERRALS)
#define SLAP_SUBENTRIES(be)	((be)->be_flags & SLAP_BFLAG_SUBENTRIES)
#define SLAP_MONITOR(be)	((be)->be_flags & SLAP_BFLAG_MONITOR)
#define SLAP_DYNAMIC(be)	((be)->be_flags & SLAP_BFLAG_DYNAMIC)

	slap_mask_t	be_restrictops;		/* restriction operations */
#define SLAP_RESTRICT_OP_ADD		0x0001U
#define	SLAP_RESTRICT_OP_BIND		0x0002U
#define SLAP_RESTRICT_OP_COMPARE	0x0004U
#define SLAP_RESTRICT_OP_DELETE		0x0008U
#define	SLAP_RESTRICT_OP_EXTENDED	0x0010U
#define SLAP_RESTRICT_OP_MODIFY		0x0020U
#define SLAP_RESTRICT_OP_RENAME		0x0040U
#define SLAP_RESTRICT_OP_SEARCH		0x0080U

#define SLAP_RESTRICT_OP_READS	\
	( SLAP_RESTRICT_OP_COMPARE	\
	| SLAP_RESTRICT_OP_SEARCH )
#define SLAP_RESTRICT_OP_WRITES	\
	( SLAP_RESTRICT_OP_ADD    \
	| SLAP_RESTRICT_OP_DELETE \
	| SLAP_RESTRICT_OP_MODIFY \
	| SLAP_RESTRICT_OP_RENAME )

#define SLAP_ALLOW_BIND_V2			0x0001U	/* LDAPv2 bind */
#define SLAP_ALLOW_BIND_ANON_CRED	0x0002U /* cred should be empty */
#define SLAP_ALLOW_BIND_ANON_DN		0x0004U /* dn should be empty */

#define SLAP_ALLOW_UPDATE_ANON		0x0008U /* allow anonymous updates */

#define SLAP_DISALLOW_BIND_ANON		0x0001U /* no anonymous */
#define SLAP_DISALLOW_BIND_SIMPLE	0x0002U	/* simple authentication */
#define SLAP_DISALLOW_BIND_SIMPLE_UNPROTECTED \
									0x0004U	/* unprotected simple auth */
#define SLAP_DISALLOW_BIND_KRBV4	0x0008U /* Kerberos V4 authentication */

#define SLAP_DISALLOW_TLS_2_ANON	0x0010U /* StartTLS -> Anonymous */
#define SLAP_DISALLOW_TLS_AUTHC		0x0020U	/* TLS while authenticated */

#define SLAP_DISALLOW_AUX_WO_CR		0x4000U

	slap_mask_t	be_requires;	/* pre-operation requirements */
#define SLAP_REQUIRE_BIND		0x0001U	/* bind before op */
#define SLAP_REQUIRE_LDAP_V3	0x0002U	/* LDAPv3 before op */
#define SLAP_REQUIRE_AUTHC		0x0004U	/* authentication before op */
#define SLAP_REQUIRE_SASL		0x0008U	/* SASL before op  */
#define SLAP_REQUIRE_STRONG		0x0010U	/* strong authentication before op */

	/* Required Security Strength Factor */
	slap_ssf_set_t be_ssf_set;

	/* these should be renamed from be_ to bd_ */
	BerVarray	be_suffix;	/* the DN suffixes of data in this backend */
	BerVarray	be_nsuffix;	/* the normalized DN suffixes in this backend */
	struct berval be_schemadn;	/* per-backend subschema subentry DN */
	struct berval be_schemandn;	/* normalized subschema DN */
	struct berval be_rootdn;	/* the magic "root" name (DN) for this db */
	struct berval be_rootndn;	/* the magic "root" normalized name (DN) for this db */
	struct berval be_rootpw;	/* the magic "root" password for this db	*/
	unsigned int be_max_deref_depth; /* limit for depth of an alias deref  */
#define be_sizelimit	be_def_limit.lms_s_soft
#define be_timelimit	be_def_limit.lms_t_soft
	struct slap_limits_set be_def_limit; /* default limits */
	struct slap_limits **be_limits; /* regex-based size and time limits */
	AccessControl *be_acl;	/* access control list for this backend	   */
	slap_access_t	be_dfltaccess;	/* access given if no acl matches	   */
	struct slap_replica_info **be_replica;	/* replicas of this backend (in master)	*/
	char	*be_replogfile;	/* replication log file (in master)	   */
	struct berval be_update_ndn;	/* allowed to make changes (in replicas) */
	BerVarray	be_update_refs;	/* where to refer modifying clients to */
	char	*be_realm;
	void	*be_private;	/* anything the backend database needs 	   */

	void    *be_pb;         /* Netscape plugin */
};

struct slap_conn;
struct slap_op;

/* Backend function typedefs */
typedef int (BI_init) LDAP_P((BackendInfo *bi));
typedef int (BI_config) LDAP_P((BackendInfo *bi,
	const char *fname, int lineno,
	int argc, char **argv));
typedef int (BI_open) LDAP_P((BackendInfo *bi));
typedef int (BI_close) LDAP_P((BackendInfo *bi));
typedef int (BI_destroy) LDAP_P((BackendInfo *bi));

typedef int (BI_db_init) LDAP_P((Backend *bd));
typedef int (BI_db_config) LDAP_P((Backend *bd,
	const char *fname, int lineno,
	int argc, char **argv));
typedef int (BI_db_open) LDAP_P((Backend *bd));
typedef int (BI_db_close) LDAP_P((Backend *bd));
typedef int (BI_db_destroy) LDAP_P((Backend *bd));

typedef int (BI_op_bind)  LDAP_P(( BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *dn, struct berval *ndn, int method,
		struct berval *cred, struct berval *edn ));
typedef int (BI_op_unbind) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o ));
typedef int (BI_op_search) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *base, struct berval *nbase,
		int scope, int deref,
		int slimit, int tlimit,
		Filter *f, struct berval *filterstr,
		AttributeName *attrs, int attrsonly));
typedef int (BI_op_compare)LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *dn, struct berval *ndn,
		AttributeAssertion *ava));
typedef int (BI_op_modify) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *dn, struct berval *ndn,
		Modifications *m));
typedef int (BI_op_modrdn) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *dn, struct berval *ndn,
		struct berval *newrdn, struct berval *nnewrdn,
		int deleteoldrdn,
		struct berval *newSup, struct berval *nnewSup ));
typedef int (BI_op_add)    LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		Entry *e));
typedef int (BI_op_delete) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *dn, struct berval *ndn));
typedef int (BI_op_abandon) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		ber_int_t msgid));
typedef int (BI_op_cancel) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		ber_int_t msgid));

typedef int (BI_op_extended) LDAP_P((
	BackendDB		*be,
	struct slap_conn	*conn,
	struct slap_op		*op,
	const char		*reqoid,
	struct berval * reqdata,
	char		**rspoid,
	struct berval ** rspdata,
	LDAPControl *** rspctrls,
	const char **	text,
	BerVarray *refs ));

typedef int (BI_entry_release_rw) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		Entry *e, int rw));

typedef int (BI_chk_referrals) LDAP_P((BackendDB *bd,
		struct slap_conn *c, struct slap_op *o,
		struct berval *dn, struct berval *ndn,
		const char **text ));

typedef int (BI_acl_group)  LDAP_P((Backend *bd,
		struct slap_conn *c, struct slap_op *o,
		Entry *e,
		struct berval *bdn,
		struct berval *edn,
		ObjectClass *group_oc,
		AttributeDescription *group_at ));

typedef int (BI_acl_attribute)  LDAP_P((Backend *bd,
		struct slap_conn *c, struct slap_op *o,
		Entry *e, struct berval *edn,
		AttributeDescription *entry_at,
		BerVarray *vals ));

typedef int (BI_operational)  LDAP_P((Backend *bd,
		struct slap_conn *c, struct slap_op *o,
		Entry *e, AttributeName *attrs, int opattrs, Attribute **a ));

typedef int (BI_has_subordinates) LDAP_P((Backend *bd,
		struct slap_conn *c, struct slap_op *o,
	        Entry *e, int *has_subordinates ));

typedef int (BI_connection_init) LDAP_P((BackendDB *bd,
		struct slap_conn *c));
typedef int (BI_connection_destroy) LDAP_P((BackendDB *bd,
		struct slap_conn *c));

typedef int (BI_tool_entry_open) LDAP_P(( BackendDB *be, int mode ));
typedef int (BI_tool_entry_close) LDAP_P(( BackendDB *be ));
typedef ID (BI_tool_entry_first) LDAP_P(( BackendDB *be ));
typedef ID (BI_tool_entry_next) LDAP_P(( BackendDB *be ));
typedef Entry* (BI_tool_entry_get) LDAP_P(( BackendDB *be, ID id ));
typedef ID (BI_tool_entry_put) LDAP_P(( BackendDB *be, Entry *e, 
			struct berval *text ));
typedef int (BI_tool_entry_reindex) LDAP_P(( BackendDB *be, ID id ));
typedef int (BI_tool_sync) LDAP_P(( BackendDB *be ));

struct slap_backend_info {
	char	*bi_type; /* type of backend */

	/*
	 * per backend type routines:
	 * bi_init: called to allocate a backend_info structure,
	 *		called once BEFORE configuration file is read.
	 *		bi_init() initializes this structure hence is
	 *		called directly from be_initialize()
	 * bi_config: called per 'backend' specific option
	 *		all such options must before any 'database' options
	 *		bi_config() is called only from read_config()
	 * bi_open: called to open each database, called
	 *		once AFTER configuration file is read but
	 *		BEFORE any bi_db_open() calls.
	 *		bi_open() is called from backend_startup()
	 * bi_close: called to close each database, called
	 *		once during shutdown after all bi_db_close calls.
	 *		bi_close() is called from backend_shutdown()
	 * bi_destroy: called to destroy each database, called
	 *		once during shutdown after all bi_db_destroy calls.
	 *		bi_destory() is called from backend_destroy()
	 */
	BI_init	*bi_init;
	BI_config	*bi_config;
	BI_open *bi_open;
	BI_close	*bi_close;
	BI_destroy	*bi_destroy;

	/*
	 * per database routines:
	 * bi_db_init: called to initialize each database,
	 *	called upon reading 'database <type>' 
	 *	called only from backend_db_init()
	 * bi_db_config: called to configure each database,
	 *  called per database to handle per database options
	 *	called only from read_config()
	 * bi_db_open: called to open each database
	 *	called once per database immediately AFTER bi_open()
	 *	calls but before daemon startup.
	 *  called only by backend_startup()
	 * bi_db_close: called to close each database
	 *	called once per database during shutdown but BEFORE
	 *  any bi_close call.
	 *  called only by backend_shutdown()
	 * bi_db_destroy: called to destroy each database
	 *  called once per database during shutdown AFTER all
	 *  bi_close calls but before bi_destory calls.
	 *  called only by backend_destory()
	 */
	BI_db_init	*bi_db_init;
	BI_db_config	*bi_db_config;
	BI_db_open	*bi_db_open;
	BI_db_close	*bi_db_close;
	BI_db_destroy	*bi_db_destroy;

	/* LDAP Operations Handling Routines */
	BI_op_bind	*bi_op_bind;
	BI_op_unbind	*bi_op_unbind;
	BI_op_search	*bi_op_search;
	BI_op_compare	*bi_op_compare;
	BI_op_modify	*bi_op_modify;
	BI_op_modrdn	*bi_op_modrdn;
	BI_op_add	*bi_op_add;
	BI_op_delete	*bi_op_delete;
	BI_op_abandon	*bi_op_abandon;
	BI_op_cancel	*bi_op_cancel;

	/* Extended Operations Helper */
	BI_op_extended	*bi_extended;

	/* Auxilary Functions */
	BI_entry_release_rw	*bi_entry_release_rw;
	BI_chk_referrals	*bi_chk_referrals;

	BI_acl_group	*bi_acl_group;
	BI_acl_attribute	*bi_acl_attribute;

	BI_operational	*bi_operational;
	BI_has_subordinates	*bi_has_subordinates;

	BI_connection_init	*bi_connection_init;
	BI_connection_destroy	*bi_connection_destroy;

	/* hooks for slap tools */
	BI_tool_entry_open	*bi_tool_entry_open;
	BI_tool_entry_close	*bi_tool_entry_close;
	BI_tool_entry_first	*bi_tool_entry_first;
	BI_tool_entry_next	*bi_tool_entry_next;
	BI_tool_entry_get	*bi_tool_entry_get;
	BI_tool_entry_put	*bi_tool_entry_put;
	BI_tool_entry_reindex	*bi_tool_entry_reindex;
	BI_tool_sync		*bi_tool_sync;

#define SLAP_INDEX_ADD_OP		0x0001
#define SLAP_INDEX_DELETE_OP	0x0002

	char **bi_controls;		/* supported controls */

	unsigned int bi_nDB;	/* number of databases of this type */
	void	*bi_private;	/* anything the backend type needs */
};

#define c_authtype	c_authz.sai_method
#define c_authmech	c_authz.sai_mech
#define c_dn		c_authz.sai_dn
#define c_ndn		c_authz.sai_ndn
#define c_ssf			c_authz.sai_ssf
#define c_transport_ssf	c_authz.sai_transport_ssf
#define c_tls_ssf		c_authz.sai_tls_ssf
#define c_sasl_ssf		c_authz.sai_sasl_ssf

#define o_authtype	o_authz.sai_method
#define o_authmech	o_authz.sai_mech
#define o_dn		o_authz.sai_dn
#define o_ndn		o_authz.sai_ndn
#define o_ssf			o_authz.sai_ssf
#define o_transport_ssf	o_authz.sai_transport_ssf
#define o_tls_ssf		o_authz.sai_tls_ssf
#define o_sasl_ssf		o_authz.sai_sasl_ssf

typedef void (slap_response)( struct slap_conn *, struct slap_op *,
	ber_tag_t, ber_int_t, ber_int_t, const char *, const char *,
	BerVarray, const char *, struct berval *,
	struct berval *, LDAPControl ** );

typedef void (slap_sresult)( struct slap_conn *, struct slap_op *,
	ber_int_t, const char *, const char *, BerVarray,
	LDAPControl **, int nentries);

typedef int (slap_sendentry)( BackendDB *, struct slap_conn *,
	struct slap_op *, Entry *, AttributeName *, int, LDAPControl **);

typedef int (slap_sendreference)( BackendDB *, struct slap_conn *,
	struct slap_op *, Entry *, BerVarray, LDAPControl **, BerVarray * );

typedef struct slap_callback {
	slap_response *sc_response;
	slap_sresult *sc_sresult;
	slap_sendentry *sc_sendentry;
	slap_sendreference *sc_sendreference;
	void *sc_private;
} slap_callback;

/*
 * Paged Results state
 */
typedef unsigned long PagedResultsCookie;
typedef struct slap_paged_state {
	Backend *ps_be;
	PagedResultsCookie ps_cookie;
	ID ps_id;
} PagedResultsState;


#if defined(LDAP_CLIENT_UPDATE) || defined(LDAP_SYNC)
#define LDAP_PSEARCH_BY_ADD		0x01
#define LDAP_PSEARCH_BY_DELETE		0x02
#define LDAP_PSEARCH_BY_PREMODIFY	0x03
#define LDAP_PSEARCH_BY_MODIFY		0x04
#define LDAP_PSEARCH_BY_SCOPEOUT	0x05

struct ldap_psearch_spec {
	struct slap_op  *op;
	struct berval   *base;
	struct berval   *nbase;
	int             scope;
	int             deref;
	int             slimit;
	int             tlimit;
	Filter          *filter;
	struct berval   *filterstr;
	AttributeName   *attrs;
	int             attrsonly;
	int             protocol;
	int             entry_count;
	LDAP_LIST_ENTRY(ldap_psearch_spec) link;
};

struct psid_entry {
	struct ldap_psearch_spec* ps;
	LDAP_LIST_ENTRY(psid_entry) link;
};
#endif


/*
 * represents an operation pending from an ldap client
 */
typedef struct slap_op {
	unsigned long o_opid;	/* id of this operation */
	unsigned long o_connid; /* id of conn initiating this op */
	struct slap_conn *o_conn;	/* connection spawning this op */

	ber_int_t	o_msgid;	/* msgid of the request */
	ber_int_t	o_protocol;	/* version of the LDAP protocol used by client */
	ber_tag_t	o_tag;		/* tag of the request */
	time_t		o_time;		/* time op was initiated */

	char *		o_extendedop;	/* extended operation OID */

	ldap_pvt_thread_t	o_tid;	/* thread handling this op */

	volatile sig_atomic_t o_abandon;	/* abandon flag */
	volatile sig_atomic_t o_cancel;		/* cancel flag */
#define SLAP_CANCEL_NONE				0x00
#define SLAP_CANCEL_REQ					0x01
#define SLAP_CANCEL_ACK					0x02
#define SLAP_CANCEL_DONE				0x03

	char o_do_not_cache;	/* don't cache from this op */
	char o_is_auth_check;	/* authorization in progress */

#define SLAP_NO_CONTROL 0
#define SLAP_NONCRITICAL_CONTROL 1
#define SLAP_CRITICAL_CONTROL 2
	char o_managedsait;
#define get_manageDSAit(op)				((int)(op)->o_managedsait)

	char o_noop;
	char o_proxy_authz;

	char o_subentries;
#define get_subentries(op)				((int)(op)->o_subentries)
	char o_subentries_visibility;
#define get_subentries_visibility(op)	((int)(op)->o_subentries_visibility)

	char o_valuesreturnfilter;

#ifdef LDAP_CONTROL_X_PERMISSIVE_MODIFY
	char o_permissive_modify;
#define get_permissiveModify(op)		((int)(op)->o_permissive_modify)
#else
#define get_permissiveModify(op)		(0)
#endif

#ifdef LDAP_CONTROL_X_DOMAIN_SCOPE
	char o_domain_scope;
#define get_domainScope(op)				((int)(op)->o_domain_scope)
#else
#define get_domainScope(op)				(0)
#endif

#ifdef LDAP_CONTROL_PAGEDRESULTS
	char o_pagedresults;
#define get_pagedresults(op)			((int)(op)->o_pagedresults)
	ber_int_t o_pagedresults_size;
	PagedResultsState o_pagedresults_state;
#else
#define get_pagedresults(op)			(0)
#endif

#ifdef LDAP_CLIENT_UPDATE
	char o_clientupdate;
	char o_clientupdate_type;
#define SLAP_LCUP_NONE				(0x0)
#define SLAP_LCUP_SYNC 				(0x1)
#define SLAP_LCUP_PERSIST			(0x2)
#define SLAP_LCUP_SYNC_AND_PERSIST		(0x3)
	ber_int_t o_clientupdate_interval;
	struct berval o_clientupdate_state;
#endif

#ifdef LDAP_SYNC
	char o_sync;
	char o_sync_mode;
#define SLAP_SYNC_NONE				(0x0)
#define SLAP_SYNC_REFRESH			(0x1)
#define SLAP_SYNC_PERSIST			(0x2)
#define SLAP_SYNC_REFRESH_AND_PERSIST		(0x3)
	struct berval o_sync_state;
#endif

#if defined(LDAP_CLIENT_UPDATE) || defined(LDAP_SYNC)
	LDAP_LIST_HEAD(lss, ldap_psearch_spec) psearch_spec;
	LDAP_LIST_HEAD(pe, psid_entry) premodify_list;
	LDAP_LIST_ENTRY(slap_op) link;
#endif

	AuthorizationInformation o_authz;

	BerElement	*o_ber;		/* ber of the request		  */
#ifdef LDAP_CONNECTIONLESS
	BerElement	*o_res_ber;	/* ber of the reply		  */
#endif
	slap_callback	*o_callback;	/* callback pointers */
	LDAPControl	**o_ctrls;	 /* controls */

	void	*o_threadctx;		/* thread pool thread context */
	void	*o_private;	/* anything the backend needs */

	LDAP_STAILQ_ENTRY(slap_op)	o_next;	/* next operation in list	  */
	ValuesReturnFilter *vrFilter; /* Structure represents ValuesReturnFilter */

#ifdef LDAP_SLAPI
	void    *o_pb;                  /* NS-SLAPI plugin */
#endif
} Operation;

typedef void (*SEND_LDAP_RESULT)(
				struct slap_conn *conn,
				struct slap_op *op,
				ber_int_t err,
				const char *matched,
				const char *text,
				BerVarray ref,
				LDAPControl **ctrls
				);

#define send_ldap_result( conn, op, err, matched, text, ref, ctrls  ) \
(*conn->c_send_ldap_result)( conn, op, err, matched, text, ref, ctrls )


typedef int (*SEND_SEARCH_ENTRY)(
				struct slap_backend_db *be,
				struct slap_conn *conn,
				struct slap_op *op,
				struct slap_entry *e,
				AttributeName *attrs,
				int attrsonly,
				LDAPControl **ctrls
				);

#define send_search_entry( be, conn, op, e, attrs, attrsonly, ctrls) \
(*conn->c_send_search_entry)( be, conn, op, e, attrs, attrsonly, ctrls)


typedef void (*SEND_SEARCH_RESULT)(
				struct slap_conn *conn,
				struct slap_op *op,
				ber_int_t err,
				const char *matched,
				const char *text,
				BerVarray   refs,
				LDAPControl **ctrls,
				int nentries
				);

#define send_search_result( conn, op, err, matched, text, refs, ctrls, nentries ) \
(*conn->c_send_search_result)( conn, op, err, matched, text, refs, ctrls, nentries )


typedef int (*SEND_SEARCH_REFERENCE)(
				struct slap_backend_db *be,
				struct slap_conn *conn,
				struct slap_op *op,
				struct slap_entry *e,
				BerVarray refs,
				LDAPControl **ctrls,
				BerVarray *v2refs
				);

#define send_search_reference( be, conn, op, e,  refs, ctrls, v2refs ) \
(*conn->c_send_search_reference)( be, conn, op, e,  refs, ctrls, v2refs )


typedef void (*SEND_LDAP_EXTENDED)(
				struct slap_conn *conn,
				struct slap_op *op,
				ber_int_t   err,
				const char  *matched,
				const char  *text,
				BerVarray   refs,
				const char      *rspoid,
				struct berval *rspdata,
				LDAPControl **ctrls
				);

#define send_ldap_extended( conn, op, err, matched, text, refs, rspoid, rspdata, ctrls) \
(*conn->c_send_ldap_extended)( conn, op, err, matched, text, refs, rspoid, rspdata, ctrls )

typedef void (*SEND_LDAP_INTERMEDIATE_RESP)(
				struct slap_conn *conn,
				struct slap_op *op,
				ber_int_t   err,
				const char  *matched,
				const char  *text,
				BerVarray   refs,
				const char      *rspoid,
				struct berval *rspdata,
				LDAPControl **ctrls
				);

#define send_ldap_intermediate_resp( conn, op, err, matched, text, refs, \
				     rspoid, rspdata, ctrls) \
	(*conn->c_send_ldap_intermediate_resp)( conn, op, err, matched, text, \
						refs, rspoid, rspdata, ctrls )

/*
 * Caches the result of a backend_group check for ACL evaluation
 */
typedef struct slap_gacl {
	struct slap_gacl *ga_next;
	Backend *ga_be;
	ObjectClass *ga_oc;
	AttributeDescription *ga_at;
	int ga_res;
	ber_len_t ga_len;
	char ga_ndn[1];
} GroupAssertion;

typedef struct slap_listener Listener;

/*
 * represents a connection from an ldap client
 */
typedef struct slap_conn {
	int			c_struct_state; /* structure management state */
	int			c_conn_state;	/* connection state */

	ldap_pvt_thread_mutex_t	c_mutex; /* protect the connection */
	Sockbuf		*c_sb;			/* ber connection stuff		  */

	/* only can be changed by connect_init */
	time_t		c_starttime;	/* when the connection was opened */
	time_t		c_activitytime;	/* when the connection was last used */
	unsigned long		c_connid;	/* id of this connection for stats*/

	struct berval	c_peer_domain;	/* DNS name of client */
	struct berval	c_peer_name;	/* peer name (trans=addr:port) */
	Listener	*c_listener;
#define c_listener_url c_listener->sl_url	/* listener URL */
#define c_sock_name c_listener->sl_name	/* sock name (trans=addr:port) */

	/* only can be changed by binding thread */
	int		c_sasl_bind_in_progress;	/* multi-op bind in progress */
	struct berval	c_sasl_bind_mech;			/* mech in progress */
	struct berval	c_sasl_dn;	/* temporary storage */

	/* authorization backend */
	Backend *c_authz_backend;

	AuthorizationInformation c_authz;
	GroupAssertion *c_groups;

	ber_int_t	c_protocol;	/* version of the LDAP protocol used by client */

	LDAP_STAILQ_HEAD(c_o, slap_op) c_ops;	/* list of operations being processed */
	LDAP_STAILQ_HEAD(c_po, slap_op) c_pending_ops;	/* list of pending operations */

	ldap_pvt_thread_mutex_t	c_write_mutex;	/* only one pdu written at a time */
	ldap_pvt_thread_cond_t	c_write_cv;		/* used to wait for sd write-ready*/

	BerElement	*c_currentber;	/* ber we're attempting to read */
	int		c_writewaiter;	/* true if writer is waiting */

#ifdef LDAP_CONNECTIONLESS
	int	c_is_udp;		/* true if this is (C)LDAP over UDP */
#endif
#ifdef HAVE_TLS
	int	c_is_tls;		/* true if this LDAP over raw TLS */
	int	c_needs_tls_accept;	/* true if SSL_accept should be called */
#endif
	int		c_sasl_layers;	 /* true if we need to install SASL i/o handlers */
	void	*c_sasl_context;	/* SASL session context */
	void	*c_sasl_extra;		/* SASL session extra stuff */
	struct slap_op	*c_sasl_bindop;	/* set to current op if it's a bind */

	PagedResultsState c_pagedresults_state; /* paged result state */

	long	c_n_ops_received;	/* num of ops received (next op_id) */
	long	c_n_ops_executing;	/* num of ops currently executing */
	long	c_n_ops_pending;	/* num of ops pending execution */
	long	c_n_ops_completed;	/* num of ops completed */

	long	c_n_get;		/* num of get calls */
	long	c_n_read;		/* num of read calls */
	long	c_n_write;		/* num of write calls */

	void    *c_pb;                  /* Netscape plugin */

	/*
	 * These are the "callbacks" that are available for back-ends to
	 * supply data back to connected clients that are connected
	 * through the "front-end".
	 */
	SEND_LDAP_RESULT c_send_ldap_result;
	SEND_SEARCH_ENTRY c_send_search_entry;
	SEND_SEARCH_RESULT c_send_search_result;
	SEND_SEARCH_REFERENCE c_send_search_reference;
	SEND_LDAP_EXTENDED c_send_ldap_extended;
#ifdef LDAP_RES_INTERMEDIATE_RESP
	SEND_LDAP_INTERMEDIATE_RESP c_send_ldap_intermediate_resp;
#endif
	
} Connection;

#if defined(LDAP_SYSLOG) && defined(LDAP_DEBUG)
#define Statslog( level, fmt, connid, opid, arg1, arg2, arg3 )	\
	do { \
		if ( ldap_debug & (level) ) \
			fprintf( stderr, (fmt), (connid), (opid), (arg1), (arg2), (arg3) );\
		if ( ldap_syslog & (level) ) \
			syslog( ldap_syslog_level, (fmt), (connid), (opid), (arg1), \
				(arg2), (arg3) ); \
	} while (0)
#define StatslogTest( level ) ((ldap_debug | ldap_syslog) & (level))
#elif defined(LDAP_DEBUG)
#define Statslog( level, fmt, connid, opid, arg1, arg2, arg3 )	\
	do { \
		if ( ldap_debug & (level) ) \
			fprintf( stderr, (fmt), (connid), (opid), (arg1), (arg2), (arg3) );\
	} while (0)
#define StatslogTest( level ) (ldap_debug & (level))
#else
#define Statslog( level, fmt, connid, opid, arg1, arg2, arg3 )
#define StatslogTest( level ) (0)
#endif

/*
 * listener; need to access it from monitor backend
 */
struct slap_listener {
	struct berval sl_url;
	struct berval sl_name;
	mode_t	sl_perms;
#ifdef HAVE_TLS
	int		sl_is_tls;
#endif
#ifdef LDAP_CONNECTIONLESS
	int	sl_is_udp;		/* UDP listener is also data port */
#endif
	int	sl_is_mute;	/* Listening is temporarily disabled */
	ber_socket_t sl_sd;
	Sockaddr sl_sa;
#define sl_addr	sl_sa.sa_in_addr
};

#ifdef SLAPD_MONITOR
/*
 * Operation indices
 */
enum {
	SLAP_OP_BIND = 0,
	SLAP_OP_UNBIND,
	SLAP_OP_ADD,
	SLAP_OP_DELETE,
	SLAP_OP_MODRDN,
	SLAP_OP_MODIFY,
	SLAP_OP_COMPARE,
	SLAP_OP_SEARCH,
	SLAP_OP_ABANDON,
	SLAP_OP_EXTENDED,
	SLAP_OP_LAST
};
#endif /* SLAPD_MONITOR */

/*
 * Better know these all around slapd
 */
#define SLAP_LDAPDN_PRETTY 0x1
#define SLAP_LDAPDN_MAXLEN 8192

/*
 * Macros for LCUP
 */
#ifdef LDAP_CLIENT_UPDATE
#define SLAP_LCUP_STATE_UPDATE_TRUE	1
#define SLAP_LCUP_STATE_UPDATE_FALSE	0
#define SLAP_LCUP_ENTRY_DELETED_TRUE	1
#define SLAP_LCUP_ENTRY_DELETED_FALSE	0
#endif /* LDAP_CLIENT_UPDATE */

#if defined(LDAP_CLIENT_UPDATE) || defined(LDAP_SYNC)
#define SLAP_SEARCH_MAX_CTRLS   10
#endif

LDAP_END_DECL

#include "proto-slap.h"

#endif /* _SLAP_H_ */
