/* mr.c - routines to manage matching rule definitions */
/* $OpenLDAP: pkg/ldap/servers/slapd/mr.c,v 1.26.2.10 2003/03/14 16:45:06 kurt Exp $ */
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

struct mindexrec {
	struct berval	mir_name;
	MatchingRule	*mir_mr;
};

static Avlnode	*mr_index = NULL;
static LDAP_SLIST_HEAD(MRList, slap_matching_rule) mr_list
	= LDAP_SLIST_HEAD_INITIALIZER(&mr_list);
static LDAP_SLIST_HEAD(MRUList, slap_matching_rule_use) mru_list
	= LDAP_SLIST_HEAD_INITIALIZER(&mru_list);

static int
mr_index_cmp(
    const void	*v_mir1,
    const void	*v_mir2
)
{
	const struct mindexrec	*mir1 = v_mir1;
	const struct mindexrec	*mir2 = v_mir2;
	int i = mir1->mir_name.bv_len - mir2->mir_name.bv_len;
	if (i) return i;
	return (strcmp( mir1->mir_name.bv_val, mir2->mir_name.bv_val ));
}

static int
mr_index_name_cmp(
    const void	*v_name,
    const void	*v_mir
)
{
	const struct berval    *name = v_name;
	const struct mindexrec *mir  = v_mir;
	int i = name->bv_len - mir->mir_name.bv_len;
	if (i) return i;
	return (strncmp( name->bv_val, mir->mir_name.bv_val, name->bv_len ));
}

MatchingRule *
mr_find( const char *mrname )
{
	struct berval bv;

	bv.bv_val = (char *)mrname;
	bv.bv_len = strlen( mrname );
	return mr_bvfind( &bv );
}

MatchingRule *
mr_bvfind( struct berval *mrname )
{
	struct mindexrec	*mir = NULL;

	if ( (mir = avl_find( mr_index, mrname, mr_index_name_cmp )) != NULL ) {
		return( mir->mir_mr );
	}
	return( NULL );
}

void
mr_destroy( void )
{
	MatchingRule *m;

	avl_free(mr_index, ldap_memfree);
	while( !LDAP_SLIST_EMPTY(&mr_list) ) {
		m = LDAP_SLIST_FIRST(&mr_list);
		LDAP_SLIST_REMOVE_HEAD(&mr_list, smr_next);
		ch_free( m->smr_str.bv_val );
		ch_free( m->smr_compat_syntaxes );
		ldap_matchingrule_free((LDAPMatchingRule *)m);
	}
}

static int
mr_insert(
    MatchingRule	*smr,
    const char		**err
)
{
	struct mindexrec	*mir;
	char			**names;

	LDAP_SLIST_INSERT_HEAD(&mr_list, smr, smr_next);

	if ( smr->smr_oid ) {
		mir = (struct mindexrec *)
			ch_calloc( 1, sizeof(struct mindexrec) );
		mir->mir_name.bv_val = smr->smr_oid;
		mir->mir_name.bv_len = strlen( smr->smr_oid );
		mir->mir_mr = smr;
		if ( avl_insert( &mr_index, (caddr_t) mir,
		                 mr_index_cmp, avl_dup_error ) ) {
			*err = smr->smr_oid;
			ldap_memfree(mir);
			return SLAP_SCHERR_MR_DUP;
		}
		/* FIX: temporal consistency check */
		mr_bvfind(&mir->mir_name);
	}
	if ( (names = smr->smr_names) ) {
		while ( *names ) {
			mir = (struct mindexrec *)
				ch_calloc( 1, sizeof(struct mindexrec) );
			mir->mir_name.bv_val = *names;
			mir->mir_name.bv_len = strlen( *names );
			mir->mir_mr = smr;
			if ( avl_insert( &mr_index, (caddr_t) mir,
			                 mr_index_cmp, avl_dup_error ) ) {
				*err = *names;
				ldap_memfree(mir);
				return SLAP_SCHERR_MR_DUP;
			}
			/* FIX: temporal consistency check */
			mr_bvfind(&mir->mir_name);
			names++;
		}
	}
	return 0;
}

int
mr_add(
    LDAPMatchingRule		*mr,
    slap_mrule_defs_rec	*def,
	MatchingRule	*amr,
    const char		**err
)
{
	MatchingRule	*smr;
	Syntax		*syn;
	Syntax		**compat_syn = NULL;
	int		code;

	if( def->mrd_compat_syntaxes ) {
		int i;
		for( i=0; def->mrd_compat_syntaxes[i]; i++ ) {
			/* just count em */
		}

		compat_syn = ch_malloc( sizeof(Syntax *) * (i+1) );

		for( i=0; def->mrd_compat_syntaxes[i]; i++ ) {
			compat_syn[i] = syn_find( def->mrd_compat_syntaxes[i] );
			if( compat_syn[i] == NULL ) {
				return SLAP_SCHERR_SYN_NOT_FOUND;
			}
		}

		compat_syn[i] = NULL;
	}

	smr = (MatchingRule *) ch_calloc( 1, sizeof(MatchingRule) );
	AC_MEMCPY( &smr->smr_mrule, mr, sizeof(LDAPMatchingRule));

	/*
	 * note: smr_bvoid uses the same memory of smr_mrule.mr_oid;
	 * smr_oidlen is #defined as smr_bvoid.bv_len
	 */
	smr->smr_bvoid.bv_val = smr->smr_mrule.mr_oid;
	smr->smr_oidlen = strlen( mr->mr_oid );
	smr->smr_usage = def->mrd_usage;
	smr->smr_compat_syntaxes = compat_syn;
	smr->smr_convert = def->mrd_convert;
	smr->smr_normalize = def->mrd_normalize;
	smr->smr_match = def->mrd_match;
	smr->smr_indexer = def->mrd_indexer;
	smr->smr_filter = def->mrd_filter;
	smr->smr_associated = amr;

	if ( smr->smr_syntax_oid ) {
		if ( (syn = syn_find(smr->smr_syntax_oid)) ) {
			smr->smr_syntax = syn;
		} else {
			*err = smr->smr_syntax_oid;
			return SLAP_SCHERR_SYN_NOT_FOUND;
		}
	} else {
		*err = "";
		return SLAP_SCHERR_MR_INCOMPLETE;
	}
	code = mr_insert(smr,err);
	return code;
}

int
register_matching_rule(
	slap_mrule_defs_rec *def )
{
	LDAPMatchingRule *mr;
	MatchingRule *amr = NULL;
	int		code;
	const char	*err;

	if( def->mrd_usage == SLAP_MR_NONE &&
		def->mrd_compat_syntaxes == NULL )
	{
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"register_matching_rule: %s not usable\n", def->mrd_desc, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY, "register_matching_rule: not usable %s\n",
		    def->mrd_desc, 0, 0 );
#endif

		return -1;
	}

	if( def->mrd_associated != NULL ) {
		amr = mr_find( def->mrd_associated );

#if 0
		/* ignore for now */

		if( amr == NULL ) {
#ifdef NEW_LOGGING
			LDAP_LOG( OPERATION, ERR,
			   "register_matching_rule: could not locate associated "
			   "matching rule %s for %s\n",
				def->mrd_associated, def->mrd_desc, 0 );
#else
			Debug( LDAP_DEBUG_ANY, "register_matching_rule: could not locate "
				"associated matching rule %s for %s\n",
				def->mrd_associated, def->mrd_desc, 0 );
#endif

			return -1;
		}
#endif
	}

	mr = ldap_str2matchingrule( def->mrd_desc, &code, &err,
		LDAP_SCHEMA_ALLOW_ALL );
	if ( !mr ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"register_matching_rule: %s before %s in %s.\n",
			ldap_scherr2str(code), err, def->mrd_desc );
#else
		Debug( LDAP_DEBUG_ANY,
			"Error in register_matching_rule: %s before %s in %s\n",
		    ldap_scherr2str(code), err, def->mrd_desc );
#endif

		return( -1 );
	}

	code = mr_add( mr, def, amr, &err );

	ldap_memfree( mr );

	if ( code ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, ERR, 
			"register_matching_rule: %s for %s in %s.\n",
			scherr2str(code), err, def->mrd_desc );
#else
		Debug( LDAP_DEBUG_ANY,
			"Error in register_matching_rule: %s for %s in %s\n",
		    scherr2str(code), err, def->mrd_desc );
#endif

		return( -1 );
	}

	return( 0 );
}

void
mru_destroy( void )
{
	MatchingRuleUse *m;

	while( !LDAP_SLIST_EMPTY(&mru_list) ) {
		m = LDAP_SLIST_FIRST(&mru_list);
		LDAP_SLIST_REMOVE_HEAD(&mru_list, smru_next);

		if ( m->smru_str.bv_val ) {
			ch_free( m->smru_str.bv_val );
		}
		/* memory borrowed from m->smru_mr */
		m->smru_oid = NULL;
		m->smru_names = NULL;
		m->smru_desc = NULL;

		/* free what's left (basically 
		 * smru_mruleuse.mru_applies_oids) */
		ldap_matchingruleuse_free((LDAPMatchingRuleUse *)m);
	}
}

int
matching_rule_use_init( void )
{
	MatchingRule	*mr;
	MatchingRuleUse	**mru_ptr = &LDAP_SLIST_FIRST(&mru_list);

#ifdef NEW_LOGGING
	LDAP_LOG( OPERATION, INFO, "matching_rule_use_init\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "matching_rule_use_init\n", 0, 0, 0 );
#endif

	LDAP_SLIST_FOREACH( mr, &mr_list, smr_next ) {
		AttributeType	*at;
		MatchingRuleUse	mru_storage, *mru = &mru_storage;

		char		**applies_oids = NULL;

		mr->smr_mru = NULL;

		/* hide rules marked as HIDE */
		if ( mr->smr_usage & SLAP_MR_HIDE ) {
			continue;
		}

		/* hide rules not marked as designed for extensibility */
		/* MR_EXT means can be used any attribute type whose
		 * syntax is same as the assertion syntax.
		 * Another mechanism is needed where rule can be used
		 * with attribute of other syntaxes.
		 * Framework doesn't support this (yet).
		 */

		if (!( ( mr->smr_usage & SLAP_MR_EXT )
			|| mr->smr_compat_syntaxes ) )
		{
			continue;
		}

		memset( mru, 0, sizeof( MatchingRuleUse ) );

		/*
		 * Note: we're using the same values of the corresponding 
		 * MatchingRule structure; maybe we'd copy them ...
		 */
		mru->smru_mr = mr;
		mru->smru_obsolete = mr->smr_obsolete;
		mru->smru_applies_oids = NULL;
		LDAP_SLIST_NEXT(mru, smru_next) = NULL;
		mru->smru_oid = mr->smr_oid;
		mru->smru_names = mr->smr_names;
		mru->smru_desc = mr->smr_desc;

#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, INFO, "    %s (%s): ", 
				mru->smru_oid, 
				mru->smru_names ? mru->smru_names[ 0 ] : "", 0 );
#else
		Debug( LDAP_DEBUG_TRACE, "    %s (%s): ", 
				mru->smru_oid, 
				mru->smru_names ? mru->smru_names[ 0 ] : "", 0 );
#endif

		at = NULL;
		for ( at_start( &at ); at; at_next( &at ) ) {
			if( at->sat_flags & SLAP_AT_HIDE ) continue;

			if( mr_usable_with_at( mr, at )) {
				ldap_charray_add( &applies_oids, at->sat_cname.bv_val );
			}
		}

		/*
		 * Note: the matchingRules that are not used
		 * by any attributeType are not listed as
		 * matchingRuleUse
		 */
		if ( applies_oids != NULL ) {
			mru->smru_applies_oids = applies_oids;
#ifdef NEW_LOGGING
			{
				char *str = ldap_matchingruleuse2str( &mru->smru_mruleuse );
				LDAP_LOG( OPERATION, INFO, "matchingRuleUse: %s\n", str, 0, 0 );
				ldap_memfree( str );
			}
#else
			{
				char *str = ldap_matchingruleuse2str( &mru->smru_mruleuse );
				Debug( LDAP_DEBUG_TRACE, "matchingRuleUse: %s\n", str, 0, 0 );
				ldap_memfree( str );
			}
#endif

			mru = (MatchingRuleUse *)ber_memalloc( sizeof( MatchingRuleUse ) );
			/* call-forward from MatchingRule to MatchingRuleUse */
			mr->smr_mru = mru;
			/* copy static data to newly allocated struct */
			*mru = mru_storage;
			/* append the struct pointer to the end of the list */
			*mru_ptr = mru;
			/* update the list head pointer */
			mru_ptr = &LDAP_SLIST_NEXT(mru,smru_next);
		}
	}

	return( 0 );
}

int mr_usable_with_at(
	MatchingRule *mr,
	AttributeType *at )
{
	if( mr->smr_usage & SLAP_MR_EXT && ( 
		mr->smr_syntax == at->sat_syntax ||
		mr == at->sat_equality || mr == at->sat_approx ) )
	{
		return 1;
	}

	if ( mr->smr_compat_syntaxes ) {
		int i;
		for( i=0; mr->smr_compat_syntaxes[i]; i++ ) {
			if( at->sat_syntax == mr->smr_compat_syntaxes[i] ) {
				return 1;
			}
		}
	}
	return 0;
}

int mr_schema_info( Entry *e )
{
	MatchingRule *mr;

	AttributeDescription *ad_matchingRules = slap_schema.si_ad_matchingRules;

	LDAP_SLIST_FOREACH(mr, &mr_list, smr_next ) {
		if ( mr->smr_usage & SLAP_MR_HIDE ) {
			/* skip hidden rules */
			continue;
		}

		if ( ! mr->smr_match ) {
			/* skip rules without matching functions */
			continue;
		}

		if ( mr->smr_str.bv_val == NULL ) {
			if ( ldap_matchingrule2bv( &mr->smr_mrule, &mr->smr_str ) == NULL ) {
				return -1;
			}
		}
#if 0
		Debug( LDAP_DEBUG_TRACE, "Merging mr [%lu] %s\n",
			mr->smr_str.bv_len, mr->smr_str.bv_val, 0 );
#endif
		if( attr_merge_one( e, ad_matchingRules, &mr->smr_str ) )
			return -1;
	}
	return 0;
}

int mru_schema_info( Entry *e )
{
	MatchingRuleUse	*mru;

	AttributeDescription *ad_matchingRuleUse 
		= slap_schema.si_ad_matchingRuleUse;

	LDAP_SLIST_FOREACH( mru, &mru_list, smru_next ) {

		assert( !( mru->smru_usage & SLAP_MR_HIDE ) );

		if ( mru->smru_str.bv_val == NULL ) {
			if ( ldap_matchingruleuse2bv( &mru->smru_mruleuse, &mru->smru_str )
					== NULL ) {
				return -1;
			}
		}

#if 0
		Debug( LDAP_DEBUG_TRACE, "Merging mru [%lu] %s\n",
			mru->smru_str.bv_len, mru->smru_str.bv_val, 0 );
#endif
		if( attr_merge_one( e, ad_matchingRuleUse, &mru->smru_str ) )
			return -1;
	}
	return 0;
}
