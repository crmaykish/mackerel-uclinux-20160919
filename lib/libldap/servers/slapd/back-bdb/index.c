/* index.c - routines for dealing with attribute indexes */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-bdb/index.c,v 1.24.2.5 2003/03/26 17:56:38 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-bdb.h"
#include "lutil_hash.h"

static char presence_keyval[LUTIL_HASH_BYTES] = {0,0,0,1};
static struct berval presence_key = {LUTIL_HASH_BYTES, presence_keyval};

static slap_mask_t index_mask(
	Backend *be,
	AttributeDescription *desc,
	struct berval *atname )
{
	AttributeType *at;
	slap_mask_t mask = 0;

	bdb_attr_mask( be->be_private, desc, &mask );

	if( mask ) {
		*atname = desc->ad_cname;
		return mask;
	}

	/* If there is a tagging option, did we ever index the base
	 * type? If so, check for mask, otherwise it's not there.
	 */
	if( slap_ad_is_tagged( desc ) && desc != desc->ad_type->sat_ad ) {
		/* has tagging option */
		bdb_attr_mask( be->be_private, desc->ad_type->sat_ad, &mask );

		if ( mask && ( mask ^ SLAP_INDEX_NOTAGS ) ) {
			*atname = desc->ad_type->sat_cname;
			return mask;
		}
	}

	/* see if supertype defined mask for its subtypes */
	for( at = desc->ad_type; at != NULL ; at = at->sat_sup ) {
		/* If no AD, we've never indexed this type */
		if ( !at->sat_ad ) continue;

		bdb_attr_mask( be->be_private, at->sat_ad, &mask );

		if ( mask && ( mask ^ SLAP_INDEX_NOSUBTYPES ) ) {
			*atname = at->sat_cname;
			return mask;
		}
	}

	return 0;
}

int bdb_index_is_indexed(
	Backend *be,
	AttributeDescription *desc )
{
	slap_mask_t mask;
	struct berval prefix;

	mask = index_mask( be, desc, &prefix );

	if( mask == 0 ) {
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	return LDAP_SUCCESS;
}

int bdb_index_param(
	Backend *be,
	AttributeDescription *desc,
	int ftype,
	DB **dbp,
	slap_mask_t *maskp,
	struct berval *prefixp )
{
	int rc;
	slap_mask_t mask;
	DB *db;

	mask = index_mask( be, desc, prefixp );

	if( mask == 0 ) {
		return LDAP_INAPPROPRIATE_MATCHING;
	}

	rc = bdb_db_cache( be, NULL, prefixp->bv_val, &db );

	if( rc != LDAP_SUCCESS ) {
		return rc;
	}

	switch( ftype ) {
	case LDAP_FILTER_PRESENT:
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_PRESENT ) ) {
			*prefixp = presence_key;
			goto done;
		}
		break;

	case LDAP_FILTER_APPROX:
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) ) {
			goto done;
		}
		/* fall thru */

	case LDAP_FILTER_EQUALITY:
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) ) {
			goto done;
		}
		break;

	case LDAP_FILTER_SUBSTRINGS:
		if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) ) {
			goto done;
		}
		break;

	default:
		return LDAP_OTHER;
	}

	return LDAP_INAPPROPRIATE_MATCHING;

done:
	*dbp = db;
	*maskp = mask;
	return LDAP_SUCCESS;
}

static int indexer(
	Backend *be,
	DB_TXN *txn,
	struct berval *atname,
	BerVarray vals,
	ID id,
	int op,
	slap_mask_t mask )
{
	int rc, i;
	const char *text;
	DB *db;
	AttributeDescription *ad = NULL;
	struct berval *keys;

	assert( mask );

	rc = bdb_db_cache( be, txn, atname->bv_val, &db );
	
	if ( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
		LDAP_LOG( INDEX, ERR, 
			"bdb_index_read: Could not open DB %s\n",
			atname->bv_val, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
			"bdb_index_read: Could not open DB %s\n",
			atname->bv_val, 0, 0 );
#endif
		return LDAP_OTHER;
	}

	rc = slap_bv2ad( atname, &ad, &text );
	if( rc != LDAP_SUCCESS ) return rc;

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_PRESENT ) ) {
		rc = bdb_key_change( be, db, txn, &presence_key, id, op );
		if( rc ) {
			goto done;
		}
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_EQUALITY ) ) {
		rc = ad->ad_type->sat_equality->smr_indexer(
			LDAP_FILTER_EQUALITY,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_equality,
			atname, vals, &keys );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = bdb_key_change( be, db, txn, &keys[i], id, op );
				if( rc ) {
					ber_bvarray_free( keys );
					goto done;
				}
			}
			ber_bvarray_free( keys );
		}
		rc = LDAP_SUCCESS;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_APPROX ) ) {
		rc = ad->ad_type->sat_approx->smr_indexer(
			LDAP_FILTER_APPROX,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_approx,
			atname, vals, &keys );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				rc = bdb_key_change( be, db, txn, &keys[i], id, op );
				if( rc ) {
					ber_bvarray_free( keys );
					goto done;
				}
			}
			ber_bvarray_free( keys );
		}

		rc = LDAP_SUCCESS;
	}

	if( IS_SLAP_INDEX( mask, SLAP_INDEX_SUBSTR ) ) {
		rc = ad->ad_type->sat_substr->smr_indexer(
			LDAP_FILTER_SUBSTRINGS,
			mask,
			ad->ad_type->sat_syntax,
			ad->ad_type->sat_substr,
			atname, vals, &keys );

		if( rc == LDAP_SUCCESS && keys != NULL ) {
			for( i=0; keys[i].bv_val != NULL; i++ ) {
				bdb_key_change( be, db, txn, &keys[i], id, op );
				if( rc ) {
					ber_bvarray_free( keys );
					goto done;
				}
			}
			ber_bvarray_free( keys );
		}

		rc = LDAP_SUCCESS;
	}

done:
	return rc;
}

static int index_at_values(
	Backend *be,
	DB_TXN *txn,
	AttributeType *type,
	struct berval *tags,
	BerVarray vals,
	ID id,
	int op )
{
	int rc;
	slap_mask_t mask = 0;

	if( type->sat_sup ) {
		/* recurse */
		rc = index_at_values( be, txn,
			type->sat_sup, tags,
			vals, id, op );

		if( rc ) return rc;
	}

	/* If this type has no AD, we've never used it before */
	if( type->sat_ad ) {
		bdb_attr_mask( be->be_private, type->sat_ad, &mask );
	}

	if( mask ) {
		rc = indexer( be, txn, &type->sat_cname,
			vals, id, op,
			mask );

		if( rc ) return rc;
	}

	if( tags->bv_len ) {
		AttributeDescription *desc;

		mask = 0;

		desc = ad_find_tags( type, tags );
		if( desc ) {
			bdb_attr_mask( be->be_private, desc, &mask );
		}

		if( mask ) {
			rc = indexer( be, txn, &desc->ad_cname,
				vals, id, op,
				mask );

			if( rc ) {
				return rc;
			}
		}
	}

	return LDAP_SUCCESS;
}

int bdb_index_values(
	Backend *be,
	DB_TXN *txn,
	AttributeDescription *desc,
	BerVarray vals,
	ID id,
	int op )
{
	int rc;

	rc = index_at_values( be, txn,
		desc->ad_type, &desc->ad_tags,
		vals, id, op );

	return rc;
}

int
bdb_index_entry(
	Backend	*be,
	DB_TXN *txn,
	int op,
	Entry	*e,
	Attribute *ap )
{
	int rc;

#ifdef NEW_LOGGING
	LDAP_LOG( INDEX, ENTRY, "index_entry: %s (%s) %ld\n",
		op == SLAP_INDEX_ADD_OP ? "add" : "del", e->e_dn, (long) e->e_id );
#else
	Debug( LDAP_DEBUG_TRACE, "=> index_entry_%s( %ld, \"%s\" )\n",
		op == SLAP_INDEX_ADD_OP ? "add" : "del",
		(long) e->e_id, e->e_dn );
#endif

	/* add each attribute to the indexes */
	for ( ; ap != NULL; ap = ap->a_next ) {
		rc = bdb_index_values( be, txn,
			ap->a_desc, ap->a_vals, e->e_id, op );

		if( rc != LDAP_SUCCESS ) {
#ifdef NEW_LOGGING
			LDAP_LOG( INDEX, ENTRY, 
				"index_entry: failure (%d)\n", rc, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE,
				"<= index_entry_%s( %ld, \"%s\" ) failure\n",
				op == SLAP_INDEX_ADD_OP ? "add" : "del",
				(long) e->e_id, e->e_dn );
#endif
			return rc;
		}
	}

#ifdef NEW_LOGGING
	LDAP_LOG( INDEX, ENTRY, "index_entry: success\n", 0, 0, 0  );
#else
	Debug( LDAP_DEBUG_TRACE, "<= index_entry_%s( %ld, \"%s\" ) success\n",
		op == SLAP_INDEX_ADD_OP ? "add" : "del",
		(long) e->e_id, e->e_dn );
#endif

	return LDAP_SUCCESS;
}
