/* schema_init.c - init builtin schema */
/* $OpenLDAP: pkg/ldap/servers/slapd/schema_prep.c,v 1.53.2.19 2003/04/12 16:01:45 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "ldap_pvt.h"
#include "ldap_pvt_uc.h"

#define OCDEBUG 0

int schema_init_done = 0;

struct slap_internal_schema slap_schema;

static int objectClassNormalize(
	slap_mask_t use,
	struct slap_syntax *syntax, /* NULL if in is asserted value */
	struct slap_matching_rule *mr,
	struct berval * in,
	struct berval * out )
{
	ObjectClass *oc = oc_bvfind( in );

	if( oc != NULL ) {
		ber_dupbv( out, &oc->soc_cname );
	} else {
		ber_dupbv( out, in );
	}

#if OCDEBUG
#ifdef NEW_LOGGING
	LDAP_LOG( CONFIG, ENTRY, 
		"< objectClassNormalize(%s, %s)\n", in->bv_val, out->bv_val, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "< objectClassNormalize(%s,%s)\n",
		in->bv_val, out->bv_val, 0 );
#endif
#endif

	return LDAP_SUCCESS;
}

static int
objectSubClassMatch(
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue )
{
	struct berval *a = (struct berval *) assertedValue;
	ObjectClass *oc = oc_bvfind( value );
	ObjectClass *asserted = oc_bvfind( a );

#if OCDEBUG
#ifdef NEW_LOGGING
	LDAP_LOG( CONFIG, ENTRY, 
		"> objectSubClassMatch(%s, %s)\n", value->bv_val, a->bv_val, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "> objectSubClassMatch(%s,%s)\n",
		value->bv_val, a->bv_val, 0 );
#endif
#endif

	if( asserted == NULL ) {
		if( OID_LEADCHAR( *a->bv_val ) ) {
			/* OID form, return FALSE */
			*matchp = 1;
			return LDAP_SUCCESS;
		}

		/* desc form, return undefined */
		return SLAPD_COMPARE_UNDEFINED;
	}

	if ( oc == NULL ) {
		/* unrecognized stored value */
		return SLAPD_COMPARE_UNDEFINED;
	}

	if( SLAP_IS_MR_VALUE_SYNTAX_MATCH( flags ) ) {
		*matchp = ( asserted != oc );
	} else {
		*matchp = !is_object_subclass( asserted, oc );
	}

#if OCDEBUG
#ifdef NEW_LOGGING
	LDAP_LOG( CONFIG, ENTRY, 
		"< objectSubClassMatch(%s, %s) = %d\n",
		value->bv_val, a->bv_val, *matchp );
#else
	Debug( LDAP_DEBUG_TRACE, "< objectSubClassMatch(%s,%s) = %d\n",
		value->bv_val, a->bv_val, *matchp );
#endif
#endif

	return LDAP_SUCCESS;
}

static int objectSubClassIndexer( 
	slap_mask_t use,
	slap_mask_t mask,
	struct slap_syntax *syntax,
	struct slap_matching_rule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keysp )
{
	int rc, noc, i;
	BerVarray ocvalues;
	
	for( noc=0; values[noc].bv_val != NULL; noc++ ) {
		/* just count em */;
	}

	/* over allocate */
	ocvalues = ch_malloc( sizeof( struct berval ) * (noc+16) );

	/* copy listed values (and termination) */
	for( i=0; i<noc; i++ ) {
		ObjectClass *oc = oc_bvfind( &values[i] );
		if( oc ) {
			ocvalues[i] = oc->soc_cname;
		} else {
			ocvalues[i] = values[i];
		}
#if OCDEBUG
#ifdef NEW_LOGGING
		LDAP_LOG( CONFIG, ENTRY, 
			"> objectSubClassIndexer(%d, %s)\n",
			i, ocvalues[i].bv_val, 0 );
#else
		Debug( LDAP_DEBUG_TRACE,
			"> objectSubClassIndexer(%d, %s)\n",
			i, ocvalues[i].bv_val, 0 );
#endif
#endif
	}

	ocvalues[i].bv_val = NULL;
	ocvalues[i].bv_len = 0;

	/* expand values */
	for( i=0; i<noc; i++ ) {
		int j;
		ObjectClass *oc = oc_bvfind( &ocvalues[i] );
		if( oc == NULL || oc->soc_sups == NULL ) continue;
		
		for( j=0; oc->soc_sups[j] != NULL; j++ ) {
			int found = 0;
			ObjectClass *sup = oc->soc_sups[j];
			int k;

			for( k=0; k<noc; k++ ) {

#if 0
#ifdef NEW_LOGGING
				LDAP_LOG( CONFIG, ENTRY, 
					"= objectSubClassIndexer(%d, %s, %s)\n",
					k, ocvalues[k].bv_val, sup->soc_cname.bv_val );
#else
				Debug( LDAP_DEBUG_TRACE,
					"= objectSubClassIndexer(%d, %s, %s)\n",
					k, ocvalues[k].bv_val, sup->soc_cname.bv_val );
#endif
#endif
				if( bvmatch( &ocvalues[k], &sup->soc_cname ) ) {
					found++;
					break;
				}
			}

			if( !found ) {
				ocvalues = ch_realloc( ocvalues,
					sizeof( struct berval ) * (noc+2) );

				assert( k == noc );

				ocvalues[noc] = sup->soc_cname;

				assert( ocvalues[noc].bv_val );
				assert( ocvalues[noc].bv_len );

				noc++;

				ocvalues[noc].bv_len = 0;
				ocvalues[noc].bv_val = NULL;

#if OCDEBUG
#ifdef NEW_LOGGING
				LDAP_LOG( CONFIG, ENTRY, 
					"< objectSubClassIndexer(%d, %d, %s)\n",
					i, k, sup->soc_cname.bv_val );
#else
				Debug( LDAP_DEBUG_TRACE,
					"< objectSubClassIndexer(%d, %d, %s)\n",
					i, k, sup->soc_cname.bv_val );
#endif
#endif
			}
		}
	}

#if 0
#ifdef NEW_LOGGING
	LDAP_LOG( CONFIG, ENTRY, 
		"< objectSubClassIndexer(%d)\n", noc, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "< objectSubClassIndexer(%d)\n",
		noc, 0, 0 );
#endif
#endif

	rc = octetStringIndexer( use, mask, syntax, mr,
		prefix, ocvalues, keysp );

	ch_free( ocvalues );
	return rc;
}

/* Index generation function */
static int objectSubClassFilter(
	slap_mask_t use,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *prefix,
	void * assertedValue,
	BerVarray *keysp )
{
#if OCDEBUG
	struct berval *bv = (struct berval *) assertedValue;
	ObjectClass *oc = oc_bvfind( bv );
	if( oc ) {
		bv = &oc->soc_cname;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( CONFIG, ENTRY, 
		"< objectSubClassFilter(%s)\n", bv->bv_val, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "< objectSubClassFilter(%s)\n",
		bv->bv_val, 0, 0 );
#endif
#endif

	return octetStringFilter( use, flags, syntax, mr,
		prefix, assertedValue, keysp );
}

static ObjectClassSchemaCheckFN rootDseObjectClass;
static ObjectClassSchemaCheckFN aliasObjectClass;
static ObjectClassSchemaCheckFN referralObjectClass;
static ObjectClassSchemaCheckFN subentryObjectClass;
static ObjectClassSchemaCheckFN dynamicObjectClass;

static struct slap_schema_oc_map {
	char *ssom_name;
	char *ssom_defn;
	ObjectClassSchemaCheckFN *ssom_check;
	slap_mask_t ssom_flags;
	size_t ssom_offset;
} oc_map[] = {
	{ "top", "( 2.5.6.0 NAME 'top' "
			"DESC 'top of the superclass chain' "
			"ABSTRACT MUST objectClass )",
		0, 0, offsetof(struct slap_internal_schema, si_oc_top) },
	{ "extensibleObject", "( 1.3.6.1.4.1.1466.101.120.111 "
			"NAME 'extensibleObject' "
			"DESC 'RFC2252: extensible object' "
			"SUP top AUXILIARY )",
		0, SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_extensibleObject) },
	{ "alias", "( 2.5.6.1 NAME 'alias' "
			"DESC 'RFC2256: an alias' "
			"SUP top STRUCTURAL "
			"MUST aliasedObjectName )",
		aliasObjectClass, SLAP_OC_ALIAS|SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_alias) },
	{ "referral", "( 2.16.840.1.113730.3.2.6 NAME 'referral' "
			"DESC 'namedref: named subordinate referral' "
			"SUP top STRUCTURAL MUST ref )",
		referralObjectClass, SLAP_OC_REFERRAL|SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_referral) },
	{ "LDAProotDSE", "( 1.3.6.1.4.1.4203.1.4.1 "
			"NAME ( 'OpenLDAProotDSE' 'LDAProotDSE' ) "
			"DESC 'OpenLDAP Root DSE object' "
			"SUP top STRUCTURAL MAY cn )",
		rootDseObjectClass, SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_rootdse) },
	{ "subentry", "( 2.5.17.0 NAME 'subentry' "
			"SUP top STRUCTURAL "
			"MUST ( cn $ subtreeSpecification ) )",
		subentryObjectClass, SLAP_OC_SUBENTRY|SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_subentry) },
	{ "subschema", "( 2.5.20.1 NAME 'subschema' "
		"DESC 'RFC2252: controlling subschema (sub)entry' "
		"AUXILIARY "
		"MAY ( dITStructureRules $ nameForms $ ditContentRules $ "
			"objectClasses $ attributeTypes $ matchingRules $ "
			"matchingRuleUse ) )",
		subentryObjectClass, SLAP_OC_OPERATIONAL,
		offsetof(struct slap_internal_schema, si_oc_subschema) },
	{ "monitor", "( 1.3.6.1.4.1.4203.666.3.2 NAME 'monitor' "
		"DESC 'OpenLDAP system monitoring' "
		"STRUCTURAL "
		"MUST cn )",
		0, SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof(struct slap_internal_schema, si_oc_monitor) },
#ifdef LDAP_DEVEL
	{ "collectiveAttributeSubentry", "( 2.5.17.2 "
			"NAME 'collectiveAttributeSubentry' "
			"AUXILIARY )",
		subentryObjectClass,
		SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY|SLAP_OC_OPERATIONAL|SLAP_OC_HIDE,
		offsetof(struct slap_internal_schema, si_oc_collectiveAttributeSubentry) },
	{ "dynamicObject", "( 1.3.6.1.4.1.1466.101.119.2 "
			"NAME 'dynamicObject' "
			"DESC 'RFC2589: Dynamic Object' "
			"SUP top AUXILIARY )",
		dynamicObjectClass, SLAP_OC_DYNAMICOBJECT,
		offsetof(struct slap_internal_schema, si_oc_dynamicObject) },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static AttributeTypeSchemaCheckFN rootDseAttribute;
static AttributeTypeSchemaCheckFN aliasAttribute;
static AttributeTypeSchemaCheckFN referralAttribute;
static AttributeTypeSchemaCheckFN subentryAttribute;
static AttributeTypeSchemaCheckFN administrativeRoleAttribute;
static AttributeTypeSchemaCheckFN dynamicAttribute;

static struct slap_schema_ad_map {
	char *ssam_name;
	char *ssam_defn;
	AttributeTypeSchemaCheckFN *ssam_check;
	slap_mask_t ssam_flags;
	slap_mr_convert_func *ssam_convert;
	slap_mr_normalize_func *ssam_normalize;
	slap_mr_match_func *ssam_match;
	slap_mr_indexer_func *ssam_indexer;
	slap_mr_filter_func *ssam_filter;
	size_t ssam_offset;
} ad_map[] = {
	{ "objectClass", "( 2.5.4.0 NAME 'objectClass' "
			"DESC 'RFC2256: object classes of the entity' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )",
		NULL, SLAP_AT_FINAL,
		NULL, objectClassNormalize, objectSubClassMatch,
			objectSubClassIndexer, objectSubClassFilter,
		offsetof(struct slap_internal_schema, si_ad_objectClass) },

	/* user entry operational attributes */
	{ "structuralObjectClass", "( 2.5.21.9 NAME 'structuralObjectClass' "
			"DESC 'X.500(93): structural object class of entry' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, objectClassNormalize, objectSubClassMatch,
			objectSubClassIndexer, objectSubClassFilter,
		offsetof(struct slap_internal_schema, si_ad_structuralObjectClass) },
	{ "createTimestamp", "( 2.5.18.1 NAME 'createTimestamp' "
			"DESC 'RFC2252: time which object was created' "
			"EQUALITY generalizedTimeMatch "
			"ORDERING generalizedTimeOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_createTimestamp) },
	{ "modifyTimestamp", "( 2.5.18.2 NAME 'modifyTimestamp' "
			"DESC 'RFC2252: time which object was last modified' "
			"EQUALITY generalizedTimeMatch "
			"ORDERING generalizedTimeOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.24 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_modifyTimestamp) },
	{ "creatorsName", "( 2.5.18.3 NAME 'creatorsName' "
			"DESC 'RFC2252: name of creator' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_creatorsName) },
	{ "modifiersName", "( 2.5.18.4 NAME 'modifiersName' "
			"DESC 'RFC2252: name of last modifier' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_modifiersName) },
	{ "hasSubordinates", "( 2.5.18.9 NAME 'hasSubordinates' "
			"DESC 'X.501: entry has children' "
			"EQUALITY booleanMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.7 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_hasSubordinates) },
	{ "subschemaSubentry", "( 2.5.18.10 NAME 'subschemaSubentry' "
			"DESC 'RFC2252: name of controlling subschema entry' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 SINGLE-VALUE "
			"NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_subschemaSubentry) },
#ifdef LDAP_DEVEL
	{ "collectiveAttributeSubentries", "( 2.5.18.12 "
			"NAME 'collectiveAttributeSubentries' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_collectiveSubentries) },
	{ "collectiveExclusions", "( 2.5.18.7 NAME 'collectiveExclusions' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_collectiveExclusions) },
#endif

	{ "entryUUID", "( 1.3.6.1.4.1.4203.666.1.6 NAME 'entryUUID' "   
			"DESC 'LCUP/LDUP: UUID of the entry' "
			"EQUALITY octetStringMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{64} "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryUUID) },
	{ "entryCSN", "( 1.3.6.1.4.1.4203.666.1.7 NAME 'entryCSN' "
			"DESC 'LCUP/LDUP: change sequence number of the entry' "
			"EQUALITY octetStringMatch "
			"ORDERING octetStringOrderingMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{64} "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryCSN) },

	{ "superiorUUID", "( 1.3.6.1.4.1.4203.666.1.11 NAME 'superiorUUID' "   
			"DESC 'LCUP/LDUP: UUID of the superior entry' "
			"EQUALITY octetStringMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{64} "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE directoryOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_superiorUUID) },

	/* root DSE attributes */
	{ "altServer", "( 1.3.6.1.4.1.1466.101.120.6 NAME 'altServer' "
			"DESC 'RFC2252: alternative servers' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_altServer) },
	{ "namingContexts", "( 1.3.6.1.4.1.1466.101.120.5 "
			"NAME 'namingContexts' "
			"DESC 'RFC2252: naming contexts' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_namingContexts) },
	{ "supportedControl", "( 1.3.6.1.4.1.1466.101.120.13 "
			"NAME 'supportedControl' "
			"DESC 'RFC2252: supported controls' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedControl) },
	{ "supportedExtension", "( 1.3.6.1.4.1.1466.101.120.7 "
			"NAME 'supportedExtension' "
			"DESC 'RFC2252: supported extended operations' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedExtension) },
	{ "supportedLDAPVersion", "( 1.3.6.1.4.1.1466.101.120.15 "
			"NAME 'supportedLDAPVersion' "
			"DESC 'RFC2252: supported LDAP versions' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedLDAPVersion) },
	{ "supportedSASLMechanisms", "( 1.3.6.1.4.1.1466.101.120.14 "
			"NAME 'supportedSASLMechanisms' "
			"DESC 'RFC2252: supported SASL mechanisms'"
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedSASLMechanisms) },
	{ "supportedFeatures", "( 1.3.6.1.4.1.4203.1.3.5 "
			"NAME 'supportedFeatures' "
			"DESC 'features supported by the server' "
			"EQUALITY objectIdentifierMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_supportedFeatures) },
	{ "monitorContext", "( 1.3.6.1.4.1.4203.666.1.10 "
			"NAME 'monitorContext' "
			"DESC 'monitor context' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_monitorContext) },
	{ "vendorName", "( 1.3.6.1.1.4 NAME 'vendorName' "
			"DESC 'RFC3045: name of implementation vendor' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_vendorName) },
	{ "vendorVersion", "( 1.3.6.1.1.5 NAME 'vendorVersion' "
			"DESC 'RFC3045: version of implementation' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"SINGLE-VALUE NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_vendorVersion) },

	/* subentry attributes */
	{ "administrativeRole", "( 2.5.18.5 NAME 'administrativeRole' "
			"EQUALITY objectIdentifierMatch "
			"USAGE directoryOperation "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.38 )",
		administrativeRoleAttribute, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_administrativeRole) },
	{ "subtreeSpecification", "( 2.5.18.6 NAME 'subtreeSpecification' "
			"SINGLE-VALUE "
			"USAGE directoryOperation "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.45 )",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_subtreeSpecification) },

	/* subschema subentry attributes */
	{ "ditStructureRules", "( 2.5.21.1 NAME 'dITStructureRules' "
			"DESC 'RFC2252: DIT structure rules' "
			"EQUALITY integerFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.17 "
			"USAGE directoryOperation ) ",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ditStructureRules) },
	{ "ditContentRules", "( 2.5.21.2 NAME 'dITContentRules' "
			"DESC 'RFC2252: DIT content rules' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.16 USAGE directoryOperation )",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ditContentRules) },
	{ "matchingRules", "( 2.5.21.4 NAME 'matchingRules' "
			"DESC 'RFC2252: matching rules' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.30 USAGE directoryOperation )",
		subentryAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_matchingRules) },
	{ "attributeTypes", "( 2.5.21.5 NAME 'attributeTypes' "
			"DESC 'RFC2252: attribute types' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.3 USAGE directoryOperation )",
		subentryAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_attributeTypes) },
	{ "objectClasses", "( 2.5.21.6 NAME 'objectClasses' "
			"DESC 'RFC2252: object classes' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.37 USAGE directoryOperation )",
		subentryAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_objectClasses) },
	{ "nameForms", "( 2.5.21.7 NAME 'nameForms' "
			"DESC 'RFC2252: name forms ' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.35 USAGE directoryOperation )",
		subentryAttribute, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_nameForms) },
	{ "matchingRuleUse", "( 2.5.21.8 NAME 'matchingRuleUse' "
			"DESC 'RFC2252: matching rule uses' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.31 USAGE directoryOperation )",
		subentryAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_matchingRuleUse) },

	{ "ldapSyntaxes", "( 1.3.6.1.4.1.1466.101.120.16 NAME 'ldapSyntaxes' "
			"DESC 'RFC2252: LDAP syntaxes' "
			"EQUALITY objectIdentifierFirstComponentMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.54 USAGE directoryOperation )",
		subentryAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ldapSyntaxes) },

	/* knowledge information */
	{ "aliasedObjectName", "( 2.5.4.1 "
			"NAME ( 'aliasedObjectName' 'aliasedEntryName' ) "
			"DESC 'RFC2256: name of aliased object' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 SINGLE-VALUE )",
		aliasAttribute, SLAP_AT_FINAL,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_aliasedObjectName) },
	{ "ref", "( 2.16.840.1.113730.3.1.34 NAME 'ref' "
			"DESC 'namedref: subordinate referral URL' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"USAGE distributedOperation )",
		referralAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_ref) },

	/* access control internals */
	{ "entry", "( 1.3.6.1.4.1.4203.1.3.1 "
			"NAME 'entry' "
			"DESC 'OpenLDAP ACL entry pseudo-attribute' "
			"SYNTAX 1.3.6.1.4.1.4203.1.1.1 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entry) },
	{ "children", "( 1.3.6.1.4.1.4203.1.3.2 "
			"NAME 'children' "
			"DESC 'OpenLDAP ACL children pseudo-attribute' "
			"SYNTAX 1.3.6.1.4.1.4203.1.1.1 "
			"SINGLE-VALUE NO-USER-MODIFICATION USAGE dSAOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_children) },
	{ "saslAuthzTo", "( 1.3.6.1.4.1.4203.666.1.8 "
			"NAME 'saslAuthzTo' "
			"DESC 'SASL proxy authorization targets' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"USAGE distributedOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_saslAuthzTo) },
	{ "saslAuthzFrom", "( 1.3.6.1.4.1.4203.666.1.9 "
			"NAME 'saslAuthzFrom' "
			"DESC 'SASL proxy authorization sources' "
			"EQUALITY caseExactMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15 "
			"USAGE distributedOperation )",
		NULL, SLAP_AT_HIDE,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_saslAuthzFrom) },
#ifdef SLAPD_ACI_ENABLED
	{ "OpenLDAPaci", "( 1.3.6.1.4.1.4203.666.1.5 "
			"NAME 'OpenLDAPaci' "
			"DESC 'OpenLDAP access control information (experimental)' "
			"EQUALITY OpenLDAPaciMatch "
			"SYNTAX 1.3.6.1.4.1.4203.666.2.1 "
			"USAGE directoryOperation )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_aci) },
#endif

#ifdef LDAP_DEVEL
	{ "entryTtl", "( 1.3.6.1.4.1.1466.101.119.3 NAME 'entryTtl' "
			"DESC 'RFC2589: entry time-to-live' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.27 SINGLE-VALUE "
			"NO-USER-MODIFICATION USAGE dSAOperation )",
		dynamicAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_entryTtl) },
	{ "dynamicSubtrees", "( 1.3.6.1.4.1.1466.101.119.4 "
			"NAME 'dynamicSubtrees' "
			"DESC 'RFC2589: dynamic subtrees' "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 NO-USER-MODIFICATION "
			"USAGE dSAOperation )",
		rootDseAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_dynamicSubtrees) },
#endif

	/* userApplication attributes (which system schema depends upon) */
	{ "distinguishedName", "( 2.5.4.49 NAME 'distinguishedName' "
			"DESC 'RFC2256: common supertype of DN attributes' "
			"EQUALITY distinguishedNameMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.12 )",
		NULL, SLAP_AT_ABSTRACT,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_distinguishedName) },
	{ "name", "( 2.5.4.41 NAME 'name' "
			"DESC 'RFC2256: common supertype of name attributes' "
			"EQUALITY caseIgnoreMatch "
			"SUBSTR caseIgnoreSubstringsMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.15{32768} )",
		NULL, SLAP_AT_ABSTRACT,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_name) },
	{ "cn", "( 2.5.4.3 NAME ( 'cn' 'commonName' ) "
			"DESC 'RFC2256: common name(s) for which the entity is known by' "
			"SUP name )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_cn) },
	{ "userPassword", "( 2.5.4.35 NAME 'userPassword' "
			"DESC 'RFC2256/2307: password of user' "
			"EQUALITY octetStringMatch "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.40{128} )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_userPassword) },

#ifdef SLAPD_AUTHPASSWD
	{ "authPassword", "( 1.3.6.1.4.1.4203.1.3.4 "
			"NAME 'authPassword' "
			"DESC 'RFC3112: authentication password attribute' "
			"EQUALITY 1.3.6.1.4.1.4203.1.2.2 "
			"SYNTAX 1.3.6.1.4.1.4203.1.1.2 )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_authPassword) },
	{ "supportedAuthPasswordSchemes", "( 1.3.6.1.4.1.4203.1.3.3 "
			"NAME 'supportedAuthPasswordSchemes' "
			"DESC 'RFC3112: supported authPassword schemes' "
			"EQUALITY caseExactIA5Match "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26{32} "
			"USAGE dSAOperation )",
		subschemaAttribute, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_authPassword) },
#endif
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
	{ "krbName", "( 1.3.6.1.4.1.250.1.32 "
			"NAME ( 'krbName' 'kerberosName' ) "
			"DESC 'Kerberos principal associated with object' "
			"EQUALITY caseIgnoreIA5Match "
			"SYNTAX 1.3.6.1.4.1.1466.115.121.1.26 "
			"SINGLE-VALUE )",
		NULL, 0,
		NULL, NULL, NULL, NULL, NULL,
		offsetof(struct slap_internal_schema, si_ad_krbName) },
#endif

	{ NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, 0 }
};

static AttributeType slap_at_undefined = {
	{ "1.1.1", NULL, NULL, 1, NULL,
		NULL, NULL, NULL, NULL,
		0, 0, 0, 1, 3, NULL }, /* LDAPAttributeType */
	{ sizeof("UNDEFINED")-1, "UNDEFINED" }, /* cname */
	NULL, /* sup */
	NULL, /* subtypes */
	NULL, NULL, NULL, NULL,	/* matching rules */
	NULL, /* syntax (this may need to be defined) */
	(AttributeTypeSchemaCheckFN *) 0, /* schema check function */
	SLAP_AT_ABSTRACT|SLAP_AT_FINAL,	/* mask */
	{ NULL }, /* next */
	NULL /* attribute description */
	/* mutex (don't know how to initialize it :) */
};

static struct slap_schema_mr_map {
	char *ssmm_name;
	size_t ssmm_offset;
} mr_map[] = {
	{ "distinguishedNameMatch",
		offsetof(struct slap_internal_schema, si_mr_distinguishedNameMatch) },
	{ "integerMatch",
		offsetof(struct slap_internal_schema, si_mr_integerMatch) },
	{ NULL, 0 }
};

static struct slap_schema_syn_map {
	char *sssm_name;
	size_t sssm_offset;
} syn_map[] = {
	{ "1.3.6.1.4.1.1466.115.121.1.40",
		offsetof(struct slap_internal_schema, si_syn_octetString) },
	{ "1.3.6.1.4.1.1466.115.121.1.12",
		offsetof(struct slap_internal_schema, si_syn_distinguishedName) },
	{ "1.3.6.1.4.1.1466.115.121.1.27",
		offsetof(struct slap_internal_schema, si_syn_integer) },
	{ NULL, 0 }
};

int
slap_schema_load( void )
{
	int i;

	for( i=0; syn_map[i].sssm_name; i++ ) {
		Syntax ** synp = (Syntax **)
			&(((char *) &slap_schema)[syn_map[i].sssm_offset]);

		assert( *synp == NULL );

		*synp = syn_find( syn_map[i].sssm_name );

		if( *synp == NULL ) {
			fprintf( stderr, "slap_schema_load: Syntax: "
				"No syntax \"%s\" defined in schema\n",
				syn_map[i].sssm_name );
			return LDAP_INVALID_SYNTAX;
		}
	}

	for( i=0; mr_map[i].ssmm_name; i++ ) {
		MatchingRule ** mrp = (MatchingRule **)
			&(((char *) &slap_schema)[mr_map[i].ssmm_offset]);

		assert( *mrp == NULL );

		*mrp = mr_find( mr_map[i].ssmm_name );

		if( *mrp == NULL ) {
			fprintf( stderr, "slap_schema_load: MatchingRule: "
				"No matching rule \"%s\" defined in schema\n",
				mr_map[i].ssmm_name );
			return LDAP_INAPPROPRIATE_MATCHING;
		}
	}

	for( i=0; ad_map[i].ssam_name; i++ ) {
		assert( ad_map[i].ssam_defn != NULL );
		{
			LDAPAttributeType *at;
			int		code;
			const char	*err;

			at = ldap_str2attributetype( ad_map[i].ssam_defn,
				&code, &err, LDAP_SCHEMA_ALLOW_ALL );
			if ( !at ) {
				fprintf( stderr,
					"slap_schema_load: AttributeType \"%s\": %s before %s\n",
					 ad_map[i].ssam_name, ldap_scherr2str(code), err );
				return code;
			}

			if ( at->at_oid == NULL ) {
				fprintf( stderr, "slap_schema_load: "
					"AttributeType \"%s\": no OID\n",
					ad_map[i].ssam_name );
				return LDAP_OTHER;
			}

			code = at_add( at, &err );
			if ( code ) {
				fprintf( stderr, "slap_schema_load: AttributeType "
					"\"%s\": %s: \"%s\"\n",
					 ad_map[i].ssam_name, scherr2str(code), err );
				return code;
			}
			ldap_memfree( at );
		}
		{
			int rc;
			const char *text;

			AttributeDescription ** adp = (AttributeDescription **)
				&(((char *) &slap_schema)[ad_map[i].ssam_offset]);

			assert( *adp == NULL );

			rc = slap_str2ad( ad_map[i].ssam_name, adp, &text );
			if( rc != LDAP_SUCCESS ) {
				fprintf( stderr, "slap_schema_load: AttributeType \"%s\": "
					"not defined in schema\n",
					ad_map[i].ssam_name );
				return rc;
			}

			if( ad_map[i].ssam_check ) {
				/* install check routine */
				(*adp)->ad_type->sat_check = ad_map[i].ssam_check;
			}
			/* install flags */
			(*adp)->ad_type->sat_flags |= ad_map[i].ssam_flags;

			/* install custom rule routine */
			if( ad_map[i].ssam_convert ||
				ad_map[i].ssam_normalize ||
				ad_map[i].ssam_match ||
				ad_map[i].ssam_indexer ||
				ad_map[i].ssam_filter )
			{
				MatchingRule *mr = ch_malloc( sizeof( MatchingRule ) );
				*mr = *(*adp)->ad_type->sat_equality;
				(*adp)->ad_type->sat_equality = mr;
				
				if( ad_map[i].ssam_convert ) {
					mr->smr_convert = ad_map[i].ssam_convert;
				}
				if( ad_map[i].ssam_normalize ) {
					mr->smr_normalize = ad_map[i].ssam_normalize;
				}
				if( ad_map[i].ssam_match ) {
					mr->smr_match = ad_map[i].ssam_match;
				}
				if( ad_map[i].ssam_indexer ) {
					mr->smr_indexer = ad_map[i].ssam_indexer;
				}
				if( ad_map[i].ssam_filter ) {
					mr->smr_filter = ad_map[i].ssam_filter;
				}
			}
		}
	}

	for( i=0; oc_map[i].ssom_name; i++ ) {
		assert( oc_map[i].ssom_defn != NULL );
		{
			LDAPObjectClass *oc;
			int		code;
			const char	*err;

			oc = ldap_str2objectclass( oc_map[i].ssom_defn, &code, &err,
				LDAP_SCHEMA_ALLOW_ALL );
			if ( !oc ) {
				fprintf( stderr, "slap_schema_load: ObjectClass "
					"\"%s\": %s before %s\n",
				 	oc_map[i].ssom_name, ldap_scherr2str(code), err );
				return code;
			}

			if ( oc->oc_oid == NULL ) {
				fprintf( stderr, "slap_schema_load: ObjectClass "
					"\"%s\": no OID\n",
					oc_map[i].ssom_name );
				return LDAP_OTHER;
			}

			code = oc_add(oc,0,&err);
			if ( code ) {
				fprintf( stderr, "slap_schema_load: ObjectClass "
					"\"%s\": %s: \"%s\"\n",
				 	oc_map[i].ssom_name, scherr2str(code), err);
				return code;
			}

			ldap_memfree(oc);
		}
		{
			ObjectClass ** ocp = (ObjectClass **)
				&(((char *) &slap_schema)[oc_map[i].ssom_offset]);

			assert( *ocp == NULL );

			*ocp = oc_find( oc_map[i].ssom_name );
			if( *ocp == NULL ) {
				fprintf( stderr, "slap_schema_load: "
					"ObjectClass \"%s\": not defined in schema\n",
					oc_map[i].ssom_name );
				return LDAP_OBJECT_CLASS_VIOLATION;
			}

			if( oc_map[i].ssom_check ) {
				/* install check routine */
				(*ocp)->soc_check = oc_map[i].ssom_check;
			}
			/* install flags */
			(*ocp)->soc_flags |= oc_map[i].ssom_flags;
		}
	}

	slap_at_undefined.sat_syntax = slap_schema.si_syn_distinguishedName;
	slap_schema.si_at_undefined = &slap_at_undefined;

	return LDAP_SUCCESS;
}

int
slap_schema_check( void )
{
	/* we should only be called once after schema_init() was called */
	assert( schema_init_done == 1 );

	/*
	 * cycle thru attributeTypes to build matchingRuleUse
	 */
	if ( matching_rule_use_init() ) {
		return LDAP_OTHER;
	}

	++schema_init_done;
	return LDAP_SUCCESS;
}

static int rootDseObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( e->e_nname.bv_len ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" only allowed in the root DSE",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	/* we should not be called for the root DSE */
	assert( 0 );
	return LDAP_SUCCESS;
}

static int aliasObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_ALIASES(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int referralObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_REFERRALS(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int subentryObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_SUBENTRIES(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( oc != slap_schema.si_oc_subentry && !is_entry_subentry( e ) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" only allowed in subentries",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int dynamicObjectClass (
	Backend *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_DYNAMIC(be) ) {
		snprintf( textbuf, textlen,
			"objectClass \"%s\" not supported in context",
			oc->soc_oid );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int rootDseAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( e->e_nname.bv_len ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the root DSE",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	/* we should not be called for the root DSE */
	assert( 0 );
	return LDAP_SUCCESS;
}

static int aliasAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_ALIASES(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_alias( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the alias",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int referralAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_REFERRALS(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_referral( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the referral",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int subentryAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_SUBENTRIES(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_subentry( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in the subentry",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}

static int administrativeRoleAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_SUBENTRIES(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	snprintf( textbuf, textlen,
		"attribute \"%s\" not supported!",
		attr->a_desc->ad_cname.bv_val );
	return LDAP_OBJECT_CLASS_VIOLATION;
}

static int dynamicAttribute (
	Backend *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen )
{
	*text = textbuf;

	if( !SLAP_DYNAMIC(be) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" not supported in context",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	if( !is_entry_dynamicObject( e ) ) {
		snprintf( textbuf, textlen,
			"attribute \"%s\" only allowed in dynamic object",
			attr->a_desc->ad_cname.bv_val );
		return LDAP_OBJECT_CLASS_VIOLATION;
	}

	return LDAP_SUCCESS;
}
