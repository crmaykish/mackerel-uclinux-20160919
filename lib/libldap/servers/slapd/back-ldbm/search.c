/* search.c - ldbm backend search function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldbm/search.c,v 1.93.2.8 2003/02/09 17:41:28 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-ldbm.h"
#include "proto-back-ldbm.h"

static ID_BLOCK	*base_candidate(
	Backend *be, Entry *e );

static ID_BLOCK	*search_candidates(
	Backend *be, Entry *e, Filter *filter,
	int scope, int deref, int manageDSAit );


int
ldbm_back_search(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    struct berval	*base,
    struct berval	*nbase,
    int		scope,
    int		deref,
    int		slimit,
    int		tlimit,
    Filter	*filter,
    struct berval	*filterstr,
    AttributeName	*attrs,
    int		attrsonly )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	int		rc, err;
	const char *text = NULL;
	time_t		stoptime;
	ID_BLOCK		*candidates;
	ID		id, cursor;
	Entry		*e;
	BerVarray		v2refs = NULL;
	Entry	*matched = NULL;
	struct berval	realbase = { 0, NULL };
	int		nentries = 0;
	int		manageDSAit = get_manageDSAit( op );
	int		cscope = LDAP_SCOPE_DEFAULT;

	struct slap_limits_set *limit = NULL;
	int isroot = 0;
		
#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "ldbm_back_search: enter\n", 0, 0, 0 );
#else
	Debug(LDAP_DEBUG_TRACE, "=> ldbm_back_search\n", 0, 0, 0);
#endif

	/* grab giant lock for reading */
	ldap_pvt_thread_rdwr_rlock(&li->li_giant_rwlock);

	if ( nbase->bv_len == 0 ) {
		/* DIT root special case */
		e = (Entry *) &slap_entry_root;

		/* need normalized dn below */
		ber_dupbv( &realbase, &e->e_nname );

		candidates = search_candidates( be, e, filter,
		    scope, deref, manageDSAit || get_domainScope(op) );

		goto searchit;
		
	} else if ( deref & LDAP_DEREF_FINDING ) {
		/* deref dn and get entry with reader lock */
		e = deref_dn_r( be, nbase, &err, &matched, &text );

		if( err == LDAP_NO_SUCH_OBJECT ) err = LDAP_REFERRAL;

	} else {
		/* get entry with reader lock */
		e = dn2entry_r( be, nbase, &matched );
		err = e != NULL ? LDAP_SUCCESS : LDAP_REFERRAL;
		text = NULL;
	}

	if ( e == NULL ) {
		struct berval matched_dn = { 0, NULL };
		BerVarray refs = NULL;

		if ( matched != NULL ) {
			BerVarray erefs;
			ber_dupbv( &matched_dn, &matched->e_name );

			erefs = is_entry_referral( matched )
				? get_entry_referrals( be, conn, op, matched )
				: NULL;

			cache_return_entry_r( &li->li_cache, matched );

			if( erefs ) {
				refs = referral_rewrite( erefs, &matched_dn,
					base, scope );

				ber_bvarray_free( erefs );
			}

		} else {
			refs = referral_rewrite( default_referral,
				NULL, base, scope );
		}

		ldap_pvt_thread_rdwr_runlock(&li->li_giant_rwlock);

		send_ldap_result( conn, op, err, matched_dn.bv_val, 
			text, refs, NULL );

		ber_bvarray_free( refs );
		ber_memfree( matched_dn.bv_val );
		return 1;
	}

	if (!manageDSAit && is_entry_referral( e ) ) {
		/* entry is a referral, don't allow add */
		struct berval matched_dn;
		BerVarray erefs;
		BerVarray refs;

		ber_dupbv( &matched_dn, &e->e_name );
		erefs = get_entry_referrals( be, conn, op, e );
		refs = NULL;

		cache_return_entry_r( &li->li_cache, e );
		ldap_pvt_thread_rdwr_runlock(&li->li_giant_rwlock);

#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, INFO,
			"ldbm_search: entry (%s) is a referral.\n",
			e->e_dn, 0, 0 );
#else
		Debug( LDAP_DEBUG_TRACE,
			"ldbm_search: entry is referral\n",
			0, 0, 0 );
#endif

		if( erefs ) {
			refs = referral_rewrite( erefs, &matched_dn,
				base, scope );

			ber_bvarray_free( erefs );
		}

		if( refs ) {
			send_ldap_result( conn, op, LDAP_REFERRAL,
				matched_dn.bv_val, NULL, refs, NULL );
			ber_bvarray_free( refs );

		} else {
			send_ldap_result( conn, op, LDAP_OTHER,
				matched_dn.bv_val,
			"bad referral object", NULL, NULL );
		}

		ber_memfree( matched_dn.bv_val );
		return 1;
	}

	if ( is_entry_alias( e ) ) {
		/* don't deref */
		deref = LDAP_DEREF_NEVER;
	}

	if ( scope == LDAP_SCOPE_BASE ) {
		cscope = LDAP_SCOPE_BASE;
		candidates = base_candidate( be, e );

	} else {
		cscope = ( scope != LDAP_SCOPE_SUBTREE )
			? LDAP_SCOPE_BASE : LDAP_SCOPE_SUBTREE;
		candidates = search_candidates( be, e, filter,
		    scope, deref, manageDSAit );
	}

	/* need normalized dn below */
	ber_dupbv( &realbase, &e->e_nname );

	cache_return_entry_r( &li->li_cache, e );

searchit:
	if ( candidates == NULL ) {
		/* no candidates */
#ifdef NEW_LOGGING
		LDAP_LOG( BACK_LDBM, INFO,
			"ldbm_search: no candidates\n" , 0, 0, 0);
#else
		Debug( LDAP_DEBUG_TRACE, "ldbm_search: no candidates\n",
			0, 0, 0 );
#endif

		send_search_result( conn, op,
			LDAP_SUCCESS,
			NULL, NULL, NULL, NULL, 0 );

		rc = 1;
		goto done;
	}

	/* if not root, get appropriate limits */
	if ( be_isroot( be, &op->o_ndn ) ) {
		isroot = 1;
	} else {
		( void ) get_limits( be, &op->o_ndn, &limit );
	}

	/* if candidates exceed to-be-checked entries, abort */
	if ( !isroot && limit->lms_s_unchecked != -1 ) {
		if ( ID_BLOCK_NIDS( candidates ) > (unsigned) limit->lms_s_unchecked ) {
			send_search_result( conn, op, LDAP_ADMINLIMIT_EXCEEDED,
					NULL, NULL, NULL, NULL, 0 );
			rc = 0;
			goto done;
		}
	}
	
	/* if root an no specific limit is required, allow unlimited search */
	if ( isroot ) {
		if ( tlimit == 0 ) {
			tlimit = -1;
		}

		if ( slimit == 0 ) {
			slimit = -1;
		}

	} else {
		/* if no limit is required, use soft limit */
		if ( tlimit <= 0 ) {
			tlimit = limit->lms_t_soft;
		
		/* if requested limit higher than hard limit, abort */
		} else if ( tlimit > limit->lms_t_hard ) {
			/* no hard limit means use soft instead */
			if ( limit->lms_t_hard == 0
					&& limit->lms_t_soft > -1
					&& tlimit > limit->lms_t_soft ) {
				tlimit = limit->lms_t_soft;
			
			/* positive hard limit means abort */
			} else if ( limit->lms_t_hard > 0 ) {
				send_search_result( conn, op, 
						LDAP_ADMINLIMIT_EXCEEDED,
						NULL, NULL, NULL, NULL, 0 );
				rc = 0; 
				goto done;
			}

			/* negative hard limit means no limit */
		}

		/* if no limit is required, use soft limit */
		if ( slimit <= 0 ) {
			slimit = limit->lms_s_soft;

		/* if requested limit higher than hard limit, abort */
		} else if ( slimit > limit->lms_s_hard ) {
			/* no hard limit means use soft instead */
			if ( limit->lms_s_hard == 0
					&& limit->lms_s_soft > -1
					&& slimit > limit->lms_s_soft ) {
				slimit = limit->lms_s_soft;

			/* positive hard limit means abort */
			} else if ( limit->lms_s_hard > 0 ) {
				send_search_result( conn, op,
						LDAP_ADMINLIMIT_EXCEEDED,
						NULL, NULL, NULL, NULL, 0 );
				rc = 0;
				goto done;
			}

			/* negative hard limit means no limit */
		}
	}

	/* compute it anyway; root does not use it */
	stoptime = op->o_time + tlimit;

	for ( id = idl_firstid( candidates, &cursor ); id != NOID;
	    id = idl_nextid( candidates, &cursor ) )
	{
		int scopeok = 0;
		int result = 0;

		/* check for abandon */
		if ( op->o_abandon ) {
			rc = 0;
			goto done;
		}

		/* check time limit */
		if ( tlimit != -1 && slap_get_time() > stoptime ) {
			send_search_result( conn, op, LDAP_TIMELIMIT_EXCEEDED,
				NULL, NULL, v2refs, NULL, nentries );
			rc = 0;
			goto done;
		}

		/* get the entry with reader lock */
		e = id2entry_r( be, id );

		if ( e == NULL ) {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_LDBM, INFO,
				"ldbm_search: candidate %ld not found.\n", id, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE,
				"ldbm_search: candidate %ld not found\n",
				id, 0, 0 );
#endif

			goto loop_continue;
		}

		if ( deref & LDAP_DEREF_SEARCHING && is_entry_alias( e ) ) {
			Entry *matched;
			int err;
			const char *text;
			
			e = deref_entry_r( be, e, &err, &matched, &text );

			if( e == NULL ) {
				e = matched;
				goto loop_continue;
			}

			if( e->e_id == id ) {
				/* circular loop */
				goto loop_continue;
			}

			/* need to skip alias which deref into scope */
			if( scope & LDAP_SCOPE_ONELEVEL ) {
				struct berval pdn;
				dnParent( &e->e_nname, &pdn );
				if ( ber_bvcmp( &pdn, &realbase ) ) {
					goto loop_continue;
				}

			} else if ( dnIsSuffix( &e->e_nname, &realbase ) ) {
				/* alias is within scope */
#ifdef NEW_LOGGING
				LDAP_LOG( BACK_LDBM, DETAIL1,
					"ldbm_search: alias \"%s\" in subtree\n", e->e_dn, 0, 0 );
#else
				Debug( LDAP_DEBUG_TRACE,
					"ldbm_search: alias \"%s\" in subtree\n",
					e->e_dn, 0, 0 );
#endif

				goto loop_continue;
			}

			scopeok = 1;
		}

		/*
		 * if it's a referral, add it to the list of referrals. only do
		 * this for non-base searches, and don't check the filter
		 * explicitly here since it's only a candidate anyway.
		 */
		if ( !manageDSAit && scope != LDAP_SCOPE_BASE &&
			is_entry_referral( e ) )
		{
			struct berval	dn;

			/* check scope */
			if ( !scopeok && scope == LDAP_SCOPE_ONELEVEL ) {
				if ( !be_issuffix( be, &e->e_nname ) ) {
					dnParent( &e->e_nname, &dn );
					scopeok = dn_match( &dn, &realbase );
				} else {
					scopeok = (realbase.bv_len == 0);
				}

			} else if ( !scopeok && scope == LDAP_SCOPE_SUBTREE ) {
				scopeok = dnIsSuffix( &e->e_nname, &realbase );

			} else {
				scopeok = 1;
			}

			if( scopeok ) {
				BerVarray erefs = get_entry_referrals(
					be, conn, op, e );
				BerVarray refs = referral_rewrite( erefs,
					&e->e_name, NULL,
					scope == LDAP_SCOPE_SUBTREE
						? LDAP_SCOPE_SUBTREE
						: LDAP_SCOPE_BASE );

				send_search_reference( be, conn, op,
					e, refs, NULL, &v2refs );

				ber_bvarray_free( refs );

			} else {
#ifdef NEW_LOGGING
				LDAP_LOG( BACK_LDBM, DETAIL2,
					"ldbm_search: candidate referral %ld scope not okay\n",
					id, 0, 0 );
#else
				Debug( LDAP_DEBUG_TRACE,
					"ldbm_search: candidate referral %ld scope not okay\n",
					id, 0, 0 );
#endif
			}

			goto loop_continue;
		}

		/* if it matches the filter and scope, send it */
		result = test_filter( be, conn, op, e, filter );

		if ( result == LDAP_COMPARE_TRUE ) {
			struct berval	dn;

			/* check scope */
			if ( !scopeok && scope == LDAP_SCOPE_ONELEVEL ) {
				if ( !be_issuffix( be, &e->e_nname ) ) {
					dnParent( &e->e_nname, &dn );
					scopeok = dn_match( &dn, &realbase );
				} else {
					scopeok = (realbase.bv_len == 0);
				}

			} else if ( !scopeok && scope == LDAP_SCOPE_SUBTREE ) {
				scopeok = dnIsSuffix( &e->e_nname, &realbase );

			} else {
				scopeok = 1;
			}

			if ( scopeok ) {
				/* check size limit */
				if ( --slimit == -1 ) {
					cache_return_entry_r( &li->li_cache, e );
					send_search_result( conn, op,
						LDAP_SIZELIMIT_EXCEEDED, NULL, NULL,
						v2refs, NULL, nentries );
					rc = 0;
					goto done;
				}

				if (e) {
					result = send_search_entry(be, conn, op,
						e, attrs, attrsonly, NULL);

					switch (result) {
					case 0:		/* entry sent ok */
						nentries++;
						break;
					case 1:		/* entry not sent */
						break;
					case -1:	/* connection closed */
						cache_return_entry_r( &li->li_cache, e );
						rc = 0;
						goto done;
					}
				}
			} else {
#ifdef NEW_LOGGING
				LDAP_LOG( BACK_LDBM, DETAIL2,
					"ldbm_search: candidate entry %ld scope not okay\n", 
					id, 0, 0 );
#else
				Debug( LDAP_DEBUG_TRACE,
					"ldbm_search: candidate entry %ld scope not okay\n",
					id, 0, 0 );
#endif
			}

		} else {
#ifdef NEW_LOGGING
			LDAP_LOG( BACK_LDBM, DETAIL2,
				"ldbm_search: candidate entry %ld does not match filter\n", 
				id, 0, 0 );
#else
			Debug( LDAP_DEBUG_TRACE,
				"ldbm_search: candidate entry %ld does not match filter\n",
				id, 0, 0 );
#endif
		}

loop_continue:
		if( e != NULL ) {
			/* free reader lock */
			cache_return_entry_r( &li->li_cache, e );
		}

		ldap_pvt_thread_yield();
	}

	send_search_result( conn, op,
		v2refs == NULL ? LDAP_SUCCESS : LDAP_REFERRAL,
		NULL, NULL, v2refs, NULL, nentries );

	rc = 0;

done:
	ldap_pvt_thread_rdwr_runlock(&li->li_giant_rwlock);

	if( candidates != NULL )
		idl_free( candidates );

	if( v2refs ) ber_bvarray_free( v2refs );
	if( realbase.bv_val ) free( realbase.bv_val );

	return rc;
}

static ID_BLOCK *
base_candidate(
    Backend	*be,
	Entry	*e )
{
	ID_BLOCK		*idl;

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, ENTRY, "base_candidate: base (%s)\n", e->e_dn, 0, 0 );
#else
	Debug(LDAP_DEBUG_TRACE, "base_candidates: base: \"%s\"\n",
		e->e_dn, 0, 0);
#endif


	idl = idl_alloc( 1 );
	idl_insert( &idl, e->e_id, 1 );

	return( idl );
}

static ID_BLOCK *
search_candidates(
    Backend	*be,
    Entry	*e,
    Filter	*filter,
    int		scope,
	int		deref,
	int		manageDSAit )
{
	ID_BLOCK		*candidates;
	Filter		f, fand, rf, af, xf;
    AttributeAssertion aa_ref, aa_alias;
	struct berval bv_ref = { sizeof("referral")-1, "referral" };
	struct berval bv_alias = { sizeof("alias")-1, "alias" };

#ifdef NEW_LOGGING
	LDAP_LOG( BACK_LDBM, DETAIL1,
		   "search_candidates: base (%s) scope %d deref %d\n",
		   e->e_ndn, scope, deref );
#else
	Debug(LDAP_DEBUG_TRACE,
		"search_candidates: base=\"%s\" s=%d d=%d\n",
		e->e_ndn, scope, deref );
#endif


	xf.f_or = filter;
	xf.f_choice = LDAP_FILTER_OR;
	xf.f_next = NULL;

	if( !manageDSAit ) {
		/* match referrals */
		rf.f_choice = LDAP_FILTER_EQUALITY;
		rf.f_ava = &aa_ref;
		rf.f_av_desc = slap_schema.si_ad_objectClass;
		rf.f_av_value = bv_ref;
		rf.f_next = xf.f_or;
		xf.f_or = &rf;
	}

	if( deref & LDAP_DEREF_SEARCHING ) {
		/* match aliases */
		af.f_choice = LDAP_FILTER_EQUALITY;
		af.f_ava = &aa_alias;
		af.f_av_desc = slap_schema.si_ad_objectClass;
		af.f_av_value = bv_alias;
		af.f_next = xf.f_or;
		xf.f_or = &af;
	}

	f.f_next = NULL;
	f.f_choice = LDAP_FILTER_AND;
	f.f_and = &fand;
	fand.f_choice = scope == LDAP_SCOPE_SUBTREE
		? SLAPD_FILTER_DN_SUBTREE
		: SLAPD_FILTER_DN_ONE;
	fand.f_dn = &e->e_nname;
	fand.f_next = xf.f_or == filter ? filter : &xf ;

	candidates = filter_candidates( be, &f );

	return( candidates );
}
