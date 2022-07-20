/* acl.c - routines to parse and check acl's */
/* $OpenLDAP: pkg/ldap/servers/slapd/acl.c,v 1.139.2.15 2003/05/22 22:22:42 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/regex.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "sets.h"
#include "lber_pvt.h"

#define ACL_BUF_SIZE 	1024	/* use most appropriate size */

/*
 * speed up compares
 */
static struct berval 
	aci_bv_entry 		= BER_BVC("entry"),
	aci_bv_br_entry		= BER_BVC("[entry]"),
	aci_bv_br_all		= BER_BVC("[all]"),
	aci_bv_access_id 	= BER_BVC("access-id"),
	aci_bv_anonymous	= BER_BVC("anonymous"),
	aci_bv_public		= BER_BVC("public"),
	aci_bv_users		= BER_BVC("users"),
	aci_bv_self 		= BER_BVC("self"),
	aci_bv_dnattr 		= BER_BVC("dnattr"),
	aci_bv_group		= BER_BVC("group"),
	aci_bv_role		= BER_BVC("role"),
	aci_bv_set		= BER_BVC("set"),
	aci_bv_set_ref		= BER_BVC("set-ref"),
	aci_bv_grant		= BER_BVC("grant"),
	aci_bv_deny		= BER_BVC("deny"),
	
	aci_bv_group_class 	= BER_BVC(SLAPD_GROUP_CLASS),
	aci_bv_group_attr 	= BER_BVC(SLAPD_GROUP_ATTR),
	aci_bv_role_class	= BER_BVC(SLAPD_ROLE_CLASS),
	aci_bv_role_attr	= BER_BVC(SLAPD_ROLE_ATTR);


static AccessControl * acl_get(
	AccessControl *ac, int *count,
	Backend *be, Operation *op,
	Entry *e,
	AttributeDescription *desc,
	int nmatches, regmatch_t *matches );

static slap_control_t acl_mask(
	AccessControl *ac, slap_mask_t *mask,
	Backend *be, Connection *conn, Operation *op,
	Entry *e,
	AttributeDescription *desc,
	struct berval *val,
	regmatch_t *matches,
	int count,
	AccessControlState *state );

#ifdef SLAPD_ACI_ENABLED
static int aci_mask(
	Backend *be,
    Connection *conn,
	Operation *op,
	Entry *e,
	AttributeDescription *desc,
	struct berval *val,
	struct berval *aci,
	regmatch_t *matches,
	slap_access_t *grant,
	slap_access_t *deny );
#endif

static int	regex_matches(
	struct berval *pat, char *str, char *buf, regmatch_t *matches);
static void	string_expand(
	struct berval *newbuf, struct berval *pattern,
	char *match, regmatch_t *matches);

typedef	struct AciSetCookie {
	Backend *be;
	Entry *e;
	Connection *conn;
	Operation *op;
} AciSetCookie;

SLAP_SET_GATHER aci_set_gather;
static int aci_match_set ( struct berval *subj, Backend *be,
    Entry *e, Connection *conn, Operation *op, int setref );

/*
 * access_allowed - check whether op->o_ndn is allowed the requested access
 * to entry e, attribute attr, value val.  if val is null, access to
 * the whole attribute is assumed (all values).
 *
 * This routine loops through all access controls and calls
 * acl_mask() on each applicable access control.
 * The loop exits when a definitive answer is reached or
 * or no more controls remain.
 *
 * returns:
 *		0	access denied
 *		1	access granted
 */

int
access_allowed(
    Backend		*be,
    Connection		*conn,
    Operation		*op,
    Entry		*e,
	AttributeDescription	*desc,
    struct berval	*val,
    slap_access_t	access,
	AccessControlState *state )
{
	int				ret = 1;
	int				count;
	AccessControl			*a = NULL;

#ifdef LDAP_DEBUG
	char accessmaskbuf[ACCESSMASK_MAXLEN];
#endif
	slap_mask_t mask;
	slap_control_t control;
	const char *attr;
	regmatch_t matches[MAXREMATCHES];
	int        st_same_attr = 0;
	int        st_initialized = 0;
	static AccessControlState state_init = ACL_STATE_INIT;

	assert( e != NULL );
	assert( desc != NULL );
	assert( access > ACL_NONE );

	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

	if( op && op->o_is_auth_check && (access == ACL_SEARCH || access == ACL_READ)) {
		access = ACL_AUTH;
	}
	if( state && state->as_recorded && state->as_vd_ad==desc) { 
		if( state->as_recorded & ACL_STATE_RECORDED_NV &&
			val == NULL )
		{
			return state->as_result;

		} else if ( state->as_recorded & ACL_STATE_RECORDED_VD &&
			val != NULL && state->as_vd_acl == NULL )
		{
			return state->as_result;
		}
		st_same_attr = 1;
	} if (state) {
		state->as_vd_ad=desc;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, ENTRY, 
		"access_allowed: %s access to \"%s\" \"%s\" requested\n",
		access2str( access ), e->e_dn, attr );
#else
	Debug( LDAP_DEBUG_ACL,
		"=> access_allowed: %s access to \"%s\" \"%s\" requested\n",
	    access2str( access ), e->e_dn, attr );
#endif

	if ( op == NULL ) {
		/* no-op call */
		goto done;
	}

	if ( be == NULL ) be = &backends[0];
	assert( be != NULL );

	/* grant database root access */
	if ( be != NULL && be_isroot( be, &op->o_ndn ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, INFO, 
			"access_allowed: conn %lu root access granted\n", 
			conn->c_connid, 0, 0 );
#else
		Debug( LDAP_DEBUG_ACL,
		    "<= root access granted\n",
			0, 0, 0 );
#endif
		goto done;
	}

	/*
	 * no-user-modification operational attributes are ignored
	 * by ACL_WRITE checking as any found here are not provided
	 * by the user
	 */
	if ( access >= ACL_WRITE && is_at_no_user_mod( desc->ad_type )
		&& desc != slap_schema.si_ad_entry
		&& desc != slap_schema.si_ad_children )
	{
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"access_allowed: conn %lu NoUserMod Operational attribute: %s "
			"access granted\n", conn->c_connid, attr , 0 );
#else
		Debug( LDAP_DEBUG_ACL, "NoUserMod Operational attribute:"
			" %s access granted\n",
			attr, 0, 0 );
#endif
		goto done;
	}

	/* use backend default access if no backend acls */
	if( be != NULL && be->be_acl == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"access_allowed: backend default %s access %s to \"%s\"\n",
		    access2str( access ),
		    be->be_dfltaccess >= access ? "granted" : "denied", 
			op->o_dn.bv_val ? op->o_dn.bv_val : "anonymous" );
#else
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: backend default %s access %s to \"%s\"\n",
			access2str( access ),
			be->be_dfltaccess >= access ? "granted" : "denied",
			op->o_dn.bv_val ? op->o_dn.bv_val : "anonymous" );
#endif
		ret = be->be_dfltaccess >= access;
		goto done;

#ifdef notdef
	/* be is always non-NULL */
	/* use global default access if no global acls */
	} else if ( be == NULL && global_acl == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"access_allowed: global default %s access %s to \"%s\"\n",
		    access2str( access ),
		    global_default_access >= access ? "granted" : "denied", 
			op->o_dn.bv_val );
#else
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: global default %s access %s to \"%s\"\n",
			access2str( access ),
			global_default_access >= access ? "granted" : "denied", op->o_dn.bv_val );
#endif
		ret = global_default_access >= access;
		goto done;
#endif
	}

	ret = 0;
	control = ACL_BREAK;

	if( st_same_attr ) {
		assert( state->as_vd_acl != NULL );

		a = state->as_vd_acl;
		mask = state->as_vd_acl_mask;
		count = state->as_vd_acl_count;
		AC_MEMCPY( matches, state->as_vd_acl_matches,
			sizeof(matches) );
		goto vd_access;

	} else {
		a = NULL;
		ACL_INIT(mask);
		count = 0;
		memset(matches, '\0', sizeof(matches));
	}

	while((a = acl_get( a, &count, be, op, e, desc,
		MAXREMATCHES, matches )) != NULL)
	{
		int i;

		for (i = 0; i < MAXREMATCHES && matches[i].rm_so > 0; i++) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				"access_allowed: match[%d]:  %d %d ",
			    i, (int)matches[i].rm_so, (int)matches[i].rm_eo );
#else
			Debug( LDAP_DEBUG_ACL, "=> match[%d]: %d %d ", i,
			    (int)matches[i].rm_so, (int)matches[i].rm_eo );
#endif
			if( matches[i].rm_so <= matches[0].rm_eo ) {
				int n;
				for ( n = matches[i].rm_so; n < matches[i].rm_eo; n++) {
					Debug( LDAP_DEBUG_ACL, "%c", e->e_ndn[n], 0, 0 );
				}
			}
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, ARGS, "\n" , 0, 0, 0 );
#else
			Debug( LDAP_DEBUG_ARGS, "\n", 0, 0, 0 );
#endif
		}

		if (state) {
			if (state->as_vi_acl == a && (state->as_recorded & ACL_STATE_RECORDED_NV)) {
				Debug( LDAP_DEBUG_ACL, "access_allowed: result from state (%s)\n", attr, 0, 0 );
				return state->as_result;
			} else if (!st_initialized) {
				Debug( LDAP_DEBUG_ACL, "access_allowed: no res from state (%s)\n", attr, 0, 0);
			    *state = state_init;
				state->as_vd_ad=desc;
				st_initialized=1;
			}
		}

vd_access:
		control = acl_mask( a, &mask, be, conn, op,
			e, desc, val, matches, count, state );

		if ( control != ACL_BREAK ) {
			break;
		}

		memset(matches, '\0', sizeof(matches));
	}

	if ( ACL_IS_INVALID( mask ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"access_allowed: conn %lu \"%s\" (%s) invalid!\n",
		    conn->c_connid, e->e_dn, attr );
#else
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: \"%s\" (%s) invalid!\n",
			e->e_dn, attr, 0 );
#endif
		ACL_INIT(mask);

	} else if ( control == ACL_BREAK ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"access_allowed: conn %lu	 no more rules\n", conn->c_connid, 0,0 );
#else
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: no more rules\n", 0, 0, 0);
#endif

		goto done;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, ENTRY, 
		"access_allowed: %s access %s by %s\n", 
		access2str( access ), ACL_GRANT( mask, access ) ? "granted" : "denied",
		accessmask2str( mask, accessmaskbuf ) );
#else
	Debug( LDAP_DEBUG_ACL,
		"=> access_allowed: %s access %s by %s\n",
		access2str( access ),
		ACL_GRANT(mask, access) ? "granted" : "denied",
		accessmask2str( mask, accessmaskbuf ) );
#endif

	ret = ACL_GRANT(mask, access);

done:
	if( state != NULL ) {
		/* If not value-dependent, save ACL in case of more attrs */
		if ( !(state->as_recorded & ACL_STATE_RECORDED_VD) )
			state->as_vi_acl = a;
		state->as_recorded |= ACL_STATE_RECORDED;
		state->as_result = ret;
	}
	return ret;
}

/*
 * acl_get - return the acl applicable to entry e, attribute
 * attr.  the acl returned is suitable for use in subsequent calls to
 * acl_access_allowed().
 */

static AccessControl *
acl_get(
	AccessControl *a,
	int			*count,
    Backend		*be,
    Operation	*op,
    Entry		*e,
	AttributeDescription *desc,
    int			nmatch,
    regmatch_t	*matches )
{
	const char *attr;
	int dnlen, patlen;

	assert( e != NULL );
	assert( count != NULL );
	assert( desc != NULL );

	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

	if( a == NULL ) {
		if( be == NULL ) {
			a = global_acl;
		} else {
			a = be->be_acl;
		}

		assert( a != NULL );

	} else {
		a = a->acl_next;
	}

	dnlen = e->e_nname.bv_len;

	for ( ; a != NULL; a = a->acl_next ) {
		(*count) ++;

		if ( a->acl_dn_pat.bv_len || ( a->acl_dn_style != ACL_STYLE_REGEX )) {
			if ( a->acl_dn_style == ACL_STYLE_REGEX ) {
#ifdef NEW_LOGGING
				LDAP_LOG( ACL, DETAIL1, 
					"acl_get: dnpat [%d] %s nsub: %d\n",
					*count, a->acl_dn_pat.bv_val, 
					(int) a->acl_dn_re.re_nsub );
#else
				Debug( LDAP_DEBUG_ACL, "=> dnpat: [%d] %s nsub: %d\n", 
					*count, a->acl_dn_pat.bv_val, (int) a->acl_dn_re.re_nsub );
#endif
				if (regexec(&a->acl_dn_re, e->e_ndn, nmatch, matches, 0))
					continue;

			} else {
#ifdef NEW_LOGGING
				LDAP_LOG( ACL, DETAIL1, "acl_get: dn [%d] %s\n",
					   *count, a->acl_dn_pat.bv_val, 0 );
#else
				Debug( LDAP_DEBUG_ACL, "=> dn: [%d] %s\n", 
					*count, a->acl_dn_pat.bv_val, 0 );
#endif
				patlen = a->acl_dn_pat.bv_len;
				if ( dnlen < patlen )
					continue;

				if ( a->acl_dn_style == ACL_STYLE_BASE ) {
					/* base dn -- entire object DN must match */
					if ( dnlen != patlen )
						continue;

				} else if ( a->acl_dn_style == ACL_STYLE_ONE ) {
					int rdnlen = -1;

					if ( dnlen <= patlen )
						continue;

					if ( !DN_SEPARATOR( e->e_ndn[dnlen - patlen - 1] ) )
						continue;

					rdnlen = dn_rdnlen( NULL, &e->e_nname );
					if ( rdnlen != dnlen - patlen - 1 )
						continue;

				} else if ( a->acl_dn_style == ACL_STYLE_SUBTREE ) {
					if ( dnlen > patlen && !DN_SEPARATOR( e->e_ndn[dnlen - patlen - 1] ) )
						continue;

				} else if ( a->acl_dn_style == ACL_STYLE_CHILDREN ) {
					if ( dnlen <= patlen )
						continue;
					if ( !DN_SEPARATOR( e->e_ndn[dnlen - patlen - 1] ) )
						continue;
				}

				if ( strcmp( a->acl_dn_pat.bv_val, e->e_ndn + dnlen - patlen ) != 0 )
					continue;
			}

#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				"acl_get: [%d] matched\n", *count, 0, 0 );
#else
			Debug( LDAP_DEBUG_ACL, "=> acl_get: [%d] matched\n",
				*count, 0, 0 );
#endif
		}

		if ( a->acl_filter != NULL ) {
			ber_int_t rc = test_filter( NULL, NULL, NULL, e, a->acl_filter );
			if ( rc != LDAP_COMPARE_TRUE ) {
				continue;
			}
		}

#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"acl_get: [%d] check attr %s\n", *count, attr ,0 );
#else
		Debug( LDAP_DEBUG_ACL, "=> acl_get: [%d] check attr %s\n",
		       *count, attr, 0);
#endif
		if ( attr == NULL || a->acl_attrs == NULL ||
			ad_inlist( desc, a->acl_attrs ) )
		{
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				"acl_get:  [%d] acl %s attr: %s\n", *count, e->e_dn, attr );
#else
			Debug( LDAP_DEBUG_ACL,
				"<= acl_get: [%d] acl %s attr: %s\n",
				*count, e->e_dn, attr );
#endif
			return a;
		}
		matches[0].rm_so = matches[0].rm_eo = -1;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, RESULTS, "acl_get: done.\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ACL, "<= acl_get: done.\n", 0, 0, 0 );
#endif
	return( NULL );
}

/*
 * Record value-dependent access control state
 */
#define ACL_RECORD_VALUE_STATE do { \
		if( state && !( state->as_recorded & ACL_STATE_RECORDED_VD )) { \
			state->as_recorded |= ACL_STATE_RECORDED_VD; \
			state->as_vd_acl = a; \
			AC_MEMCPY( state->as_vd_acl_matches, matches, \
				sizeof( state->as_vd_acl_matches )) ; \
			state->as_vd_acl_count = count; \
			state->as_vd_access = b; \
			state->as_vd_access_count = i; \
		} \
	} while( 0 )

/*
 * acl_mask - modifies mask based upon the given acl and the
 * requested access to entry e, attribute attr, value val.  if val
 * is null, access to the whole attribute is assumed (all values).
 *
 * returns	0	access NOT allowed
 *		1	access allowed
 */

static slap_control_t
acl_mask(
    AccessControl	*a,
	slap_mask_t *mask,
    Backend		*be,
    Connection	*conn,
    Operation	*op,
    Entry		*e,
	AttributeDescription *desc,
    struct berval	*val,
	regmatch_t	*matches,
	int	count,
	AccessControlState *state )
{
	int		i, odnlen, patlen;
	Access	*b;
#ifdef LDAP_DEBUG
	char accessmaskbuf[ACCESSMASK_MAXLEN];
#endif
	const char *attr;

	assert( a != NULL );
	assert( mask != NULL );
	assert( desc != NULL );

	attr = desc->ad_cname.bv_val;

	assert( attr != NULL );

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, ENTRY, 
		"acl_mask: conn %lu  access to entry \"%s\", attr \"%s\" requested\n",
		conn->c_connid, e->e_dn, attr );

	LDAP_LOG( ACL, ARGS, 
		" to %s by \"%s\", (%s) \n", val ? "value" : "all values",
		op->o_ndn.bv_val ? op->o_ndn.bv_val : "",
		accessmask2str( *mask, accessmaskbuf ) );
#else
	Debug( LDAP_DEBUG_ACL,
		"=> acl_mask: access to entry \"%s\", attr \"%s\" requested\n",
		e->e_dn, attr, 0 );

	Debug( LDAP_DEBUG_ACL,
		"=> acl_mask: to %s by \"%s\", (%s) \n",
		val ? "value" : "all values",
		op->o_ndn.bv_val ?  op->o_ndn.bv_val : "",
		accessmask2str( *mask, accessmaskbuf ) );
#endif

	if( state && ( state->as_recorded & ACL_STATE_RECORDED_VD )
		&& state->as_vd_acl == a )
	{
		b = state->as_vd_access;
		i = state->as_vd_access_count;

	} else {
		b = a->acl_access;
		i = 1;
	}

	for ( ; b != NULL; b = b->a_next, i++ ) {
		slap_mask_t oldmask, modmask;

		ACL_INVALIDATE( modmask );

		/* AND <who> clauses */
		if ( b->a_dn_pat.bv_len != 0 ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				"acl_mask: conn %lu  check a_dn_pat: %s\n",
				conn->c_connid, b->a_dn_pat.bv_val ,0 );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_dn_pat: %s\n",
				b->a_dn_pat.bv_val, 0, 0);
#endif
			/*
			 * if access applies to the entry itself, and the
			 * user is bound as somebody in the same namespace as
			 * the entry, OR the given dn matches the dn pattern
			 */
			if ( bvmatch( &b->a_dn_pat, &aci_bv_anonymous ) ) {
				if ( op->o_ndn.bv_len != 0 ) {
					continue;
				}

			} else if ( bvmatch( &b->a_dn_pat, &aci_bv_users ) ) {
				if ( op->o_ndn.bv_len == 0 ) {
					continue;
				}

			} else if ( bvmatch( &b->a_dn_pat, &aci_bv_self ) ) {
				if ( op->o_ndn.bv_len == 0 ) {
					continue;
				}
				
				if ( e->e_dn == NULL || !dn_match( &e->e_nname, &op->o_ndn ) ) {
					continue;
				}

			} else if ( b->a_dn_style == ACL_STYLE_REGEX ) {
				if ( !ber_bvccmp( &b->a_dn_pat, '*' ) ) {
					int ret = regex_matches( &b->a_dn_pat,
						op->o_ndn.bv_val, e->e_ndn, matches );

					if( ret == 0 ) {
						continue;
					}
				}

			} else {
				struct berval pat;
				int got_match = 0;

				if ( e->e_dn == NULL )
					continue;

				if ( b->a_dn_expand ) {
					struct berval bv;
					char buf[ACL_BUF_SIZE];

					bv.bv_len = sizeof( buf ) - 1;
					bv.bv_val = buf;

					string_expand(&bv, &b->a_dn_pat, 
							e->e_ndn, matches);
					if ( dnNormalize2(NULL, &bv, &pat) != LDAP_SUCCESS ) {
						/* did not expand to a valid dn */
						continue;
					}
				} else {
					pat = b->a_dn_pat;
				}

				patlen = pat.bv_len;
				odnlen = op->o_ndn.bv_len;
				if ( odnlen < patlen ) {
					goto dn_match_cleanup;

				}

				if ( b->a_dn_style == ACL_STYLE_BASE ) {
					/* base dn -- entire object DN must match */
					if ( odnlen != patlen ) {
						goto dn_match_cleanup;
					}

				} else if ( b->a_dn_style == ACL_STYLE_ONE ) {
					int rdnlen = -1;

					if ( odnlen <= patlen ) {
						goto dn_match_cleanup;
					}

					if ( !DN_SEPARATOR( op->o_ndn.bv_val[odnlen - patlen - 1] ) ) {
						goto dn_match_cleanup;
					}

					rdnlen = dn_rdnlen( NULL, &op->o_ndn );
					if ( rdnlen != odnlen - patlen - 1 ) {
						goto dn_match_cleanup;
					}

				} else if ( b->a_dn_style == ACL_STYLE_SUBTREE ) {
					if ( odnlen > patlen && !DN_SEPARATOR( op->o_ndn.bv_val[odnlen - patlen - 1] ) ) {
						goto dn_match_cleanup;
					}

				} else if ( b->a_dn_style == ACL_STYLE_CHILDREN ) {
					if ( odnlen <= patlen ) {
						goto dn_match_cleanup;
					}

					if ( !DN_SEPARATOR( op->o_ndn.bv_val[odnlen - patlen - 1] ) ) {
						goto dn_match_cleanup;
					}
				}

				got_match = !strcmp( pat.bv_val, op->o_ndn.bv_val + odnlen - patlen );

dn_match_cleanup:;
				if ( pat.bv_val != b->a_dn_pat.bv_val ) {
					free( pat.bv_val );
				}

				if ( !got_match ) {
					continue;
				}
			}
		}

		if ( b->a_sockurl_pat.bv_len ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				   "acl_mask: conn %lu  check a_sockurl_pat: %s\n",
				   conn->c_connid, b->a_sockurl_pat.bv_val , 0 );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_sockurl_pat: %s\n",
				b->a_sockurl_pat.bv_val, 0, 0 );
#endif

			if ( !ber_bvccmp( &b->a_sockurl_pat, '*' ) ) {
				if ( b->a_sockurl_style == ACL_STYLE_REGEX) {
					if (!regex_matches( &b->a_sockurl_pat, conn->c_listener_url.bv_val,
							e->e_ndn, matches ) ) 
					{
						continue;
					}
				} else {
					if ( ber_bvstrcasecmp( &b->a_sockurl_pat, &conn->c_listener_url ) != 0 )
						continue;
				}
			}
		}

		if ( b->a_domain_pat.bv_len ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				   "acl_mask: conn %lu  check a_domain_pat: %s\n",
				   conn->c_connid, b->a_domain_pat.bv_val , 0 );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_domain_pat: %s\n",
				b->a_domain_pat.bv_val, 0, 0 );
#endif
			if ( !ber_bvccmp( &b->a_domain_pat, '*' ) ) {
				if ( b->a_domain_style == ACL_STYLE_REGEX) {
					if (!regex_matches( &b->a_domain_pat, conn->c_peer_domain.bv_val,
							e->e_ndn, matches ) ) 
					{
						continue;
					}
				} else {
					char buf[ACL_BUF_SIZE];

					struct berval 	cmp = conn->c_peer_domain;
					struct berval 	pat = b->a_domain_pat;

					if ( b->a_domain_expand ) {
						struct berval bv;

						bv.bv_len = sizeof(buf) - 1;
						bv.bv_val = buf;

						string_expand(&bv, &b->a_domain_pat, e->e_ndn, matches);
						pat = bv;
					}

					if ( b->a_domain_style == ACL_STYLE_SUBTREE ) {
						int offset = cmp.bv_len - pat.bv_len;
						if ( offset < 0 ) {
							continue;
						}

						if ( offset == 1 || ( offset > 1 && cmp.bv_val[ offset - 1 ] != '.' ) ) {
							continue;
						}

						/* trim the domain */
						cmp.bv_val = &cmp.bv_val[ offset ];
						cmp.bv_len -= offset;
					}
					
					if ( ber_bvstrcasecmp( &pat, &cmp ) != 0 ) {
						continue;
					}
				}
			}
		}

		if ( b->a_peername_pat.bv_len ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				   "acl_mask: conn %lu  check a_perrname_path: %s\n",
				   conn->c_connid, b->a_peername_pat.bv_val , 0 );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_peername_path: %s\n",
				b->a_peername_pat.bv_val, 0, 0 );
#endif
			if ( !ber_bvccmp( &b->a_peername_pat, '*' ) ) {
				if ( b->a_peername_style == ACL_STYLE_REGEX) {
					if (!regex_matches( &b->a_peername_pat, conn->c_peer_name.bv_val,
							e->e_ndn, matches ) ) 
					{
						continue;
					}
				} else {
					if ( ber_bvstrcasecmp( &b->a_peername_pat, &conn->c_peer_name ) != 0 )
						continue;
				}
			}
		}

		if ( b->a_sockname_pat.bv_len ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				   "acl_mask: conn %lu  check a_sockname_path: %s\n",
				   conn->c_connid, b->a_sockname_pat.bv_val , 0 );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_sockname_path: %s\n",
				b->a_sockname_pat.bv_val, 0, 0 );
#endif
			if ( !ber_bvccmp( &b->a_sockname_pat, '*' ) ) {
				if ( b->a_sockname_style == ACL_STYLE_REGEX) {
					if (!regex_matches( &b->a_sockname_pat, conn->c_sock_name.bv_val,
							e->e_ndn, matches ) ) 
					{
						continue;
					}
				} else {
					if ( ber_bvstrcasecmp( &b->a_sockname_pat, &conn->c_sock_name ) != 0 )
						continue;
				}
			}
		}

		if ( b->a_dn_at != NULL ) {
			Attribute	*at;
			struct berval	bv;
			int rc, match = 0;
			const char *text;
			const char *attr = b->a_dn_at->ad_cname.bv_val;

			assert( attr != NULL );

			if ( op->o_ndn.bv_len == 0 ) {
				continue;
			}

#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				   "acl_mask: conn %lu  check a_dn_pat: %s\n",
				   conn->c_connid, attr , 0 );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_dn_at: %s\n",
				attr, 0, 0);
#endif
			bv = op->o_ndn;

			/* see if asker is listed in dnattr */
			for( at = attrs_find( e->e_attrs, b->a_dn_at );
				at != NULL;
				at = attrs_find( at->a_next, b->a_dn_at ) )
			{
				if( value_find_ex( b->a_dn_at,
					SLAP_MR_VALUE_NORMALIZED_MATCH, at->a_vals, &bv ) == 0 ) {
					/* found it */
					match = 1;
					break;
				}
			}

			if( match ) {
				/* have a dnattr match. if this is a self clause then
				 * the target must also match the op dn.
				 */
				if ( b->a_dn_self ) {
					/* check if the target is an attribute. */
					if ( val == NULL )
						continue;
					/* target is attribute, check if the attribute value
					 * is the op dn.
					 */
					rc = value_match( &match, b->a_dn_at,
						b->a_dn_at->ad_type->sat_equality, 0,
						val, &bv, &text );
					/* on match error or no match, fail the ACL clause */
					if (rc != LDAP_SUCCESS || match != 0 )
						continue;
				}
			} else {
				/* no dnattr match, check if this is a self clause */
				if ( ! b->a_dn_self )
					continue;

				ACL_RECORD_VALUE_STATE;
				
				/* this is a self clause, check if the target is an
				 * attribute.
				 */
				if ( val == NULL )
					continue;

				/* target is attribute, check if the attribute value
				 * is the op dn.
				 */
				rc = value_match( &match, b->a_dn_at,
					b->a_dn_at->ad_type->sat_equality, 0,
					val, &bv, &text );

				/* on match error or no match, fail the ACL clause */
				if (rc != LDAP_SUCCESS || match != 0 )
					continue;
			}
		}

		if ( b->a_group_pat.bv_len ) {
			struct berval bv;
			struct berval ndn = { 0, NULL };
			int rc;

			if ( op->o_ndn.bv_len == 0 ) {
				continue;
			}

			/* b->a_group is an unexpanded entry name, expanded it should be an 
			 * entry with objectclass group* and we test to see if odn is one of
			 * the values in the attribute group
			 */
			/* see if asker is listed in dnattr */
			if ( b->a_group_style == ACL_STYLE_REGEX ) {
				char buf[ACL_BUF_SIZE];
				bv.bv_len = sizeof(buf) - 1;
				bv.bv_val = buf; 

				string_expand( &bv, &b->a_group_pat, e->e_ndn, matches );
				if ( dnNormalize2( NULL, &bv, &ndn ) != LDAP_SUCCESS ) {
					/* did not expand to a valid dn */
					continue;
				}

				bv = ndn;

			} else {
				bv = b->a_group_pat;
			}

			rc = backend_group( be, conn, op, e, &bv, &op->o_ndn,
				b->a_group_oc, b->a_group_at );

			if ( ndn.bv_val ) free( ndn.bv_val );

			if ( rc != 0 ) {
				continue;
			}
		}

		if ( b->a_set_pat.bv_len != 0 ) {
			struct berval bv;
			char buf[ACL_BUF_SIZE];
			if( b->a_set_style == ACL_STYLE_REGEX ){
				bv.bv_len = sizeof(buf) - 1;
				bv.bv_val = buf;
				string_expand( &bv, &b->a_set_pat, e->e_ndn, matches );
			}else{
				bv = b->a_set_pat;
			}
			if (aci_match_set( &bv, be, e, conn, op, 0 ) == 0) {
				continue;
			}
		}

		if ( b->a_authz.sai_ssf ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
		  		"acl_mask: conn %lu  check a_authz.sai_ssf: ACL %u > OP %u\n",
				conn->c_connid, b->a_authz.sai_ssf, op->o_ssf  );
#else
			Debug( LDAP_DEBUG_ACL, "<= check a_authz.sai_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_ssf, op->o_ssf, 0 );
#endif
			if ( b->a_authz.sai_ssf >  op->o_ssf ) {
				continue;
			}
		}

		if ( b->a_authz.sai_transport_ssf ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
		   		"acl_mask: conn %lu  check a_authz.sai_transport_ssf: "
				"ACL %u > OP %u\n",
				conn->c_connid, b->a_authz.sai_transport_ssf, 
				op->o_transport_ssf  );
#else
			Debug( LDAP_DEBUG_ACL,
				"<= check a_authz.sai_transport_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_transport_ssf, op->o_transport_ssf, 0 );
#endif
			if ( b->a_authz.sai_transport_ssf >  op->o_transport_ssf ) {
				continue;
			}
		}

		if ( b->a_authz.sai_tls_ssf ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				"acl_mask: conn %lu  check a_authz.sai_tls_ssf: ACL %u > "
				"OP %u\n",
				conn->c_connid, b->a_authz.sai_tls_ssf, op->o_tls_ssf  );
#else
			Debug( LDAP_DEBUG_ACL,
				"<= check a_authz.sai_tls_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_tls_ssf, op->o_tls_ssf, 0 );
#endif
			if ( b->a_authz.sai_tls_ssf >  op->o_tls_ssf ) {
				continue;
			}
		}

		if ( b->a_authz.sai_sasl_ssf ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
			   "acl_mask: conn %lu check a_authz.sai_sasl_ssf: " 
			   "ACL %u > OP %u\n",
				conn->c_connid, b->a_authz.sai_sasl_ssf, op->o_sasl_ssf );
#else
			Debug( LDAP_DEBUG_ACL,
				"<= check a_authz.sai_sasl_ssf: ACL %u > OP %u\n",
				b->a_authz.sai_sasl_ssf, op->o_sasl_ssf, 0 );
#endif
			if ( b->a_authz.sai_sasl_ssf >	op->o_sasl_ssf ) {
				continue;
			}
		}

#ifdef SLAPD_ACI_ENABLED
		if ( b->a_aci_at != NULL ) {
			Attribute	*at;
			slap_access_t grant, deny, tgrant, tdeny;

			/* this case works different from the others above.
			 * since aci's themselves give permissions, we need
			 * to first check b->a_access_mask, the ACL's access level.
			 */

			if ( e->e_nname.bv_len == 0 ) {
				/* no ACIs in the root DSE */
				continue;
			}

			/* first check if the right being requested
			 * is allowed by the ACL clause.
			 */
			if ( ! ACL_GRANT( b->a_access_mask, *mask ) ) {
				continue;
			}

			/* get the aci attribute */
			at = attr_find( e->e_attrs, b->a_aci_at );
			if ( at == NULL ) {
				continue;
			}

			ACL_RECORD_VALUE_STATE;

			/* start out with nothing granted, nothing denied */
			ACL_INIT(tgrant);
			ACL_INIT(tdeny);

			/* the aci is an multi-valued attribute.  The
			 * rights are determined by OR'ing the individual
			 * rights given by the acis.
			 */
			for ( i = 0; at->a_vals[i].bv_val != NULL; i++ ) {
				if (aci_mask( be, conn, op,
					e, desc, val, &at->a_vals[i],
					matches, &grant, &deny ) != 0)
				{
					tgrant |= grant;
					tdeny |= deny;
				}
			}

			/* remove anything that the ACL clause does not allow */
			tgrant &= b->a_access_mask & ACL_PRIV_MASK;
			tdeny &= ACL_PRIV_MASK;

			/* see if we have anything to contribute */
			if( ACL_IS_INVALID(tgrant) && ACL_IS_INVALID(tdeny) ) { 
				continue;
			}

			/* this could be improved by changing acl_mask so that it can deal with
			 * by clauses that return grant/deny pairs.  Right now, it does either
			 * additive or subtractive rights, but not both at the same time.  So,
			 * we need to combine the grant/deny pair into a single rights mask in
			 * a smart way:	 if either grant or deny is "empty", then we use the
			 * opposite as is, otherwise we remove any denied rights from the grant
			 * rights mask and construct an additive mask.
			 */
			if (ACL_IS_INVALID(tdeny)) {
				modmask = tgrant | ACL_PRIV_ADDITIVE;

			} else if (ACL_IS_INVALID(tgrant)) {
				modmask = tdeny | ACL_PRIV_SUBSTRACTIVE;

			} else {
				modmask = (tgrant & ~tdeny) | ACL_PRIV_ADDITIVE;
			}

		} else
#endif
		{
			modmask = b->a_access_mask;
		}

#ifdef NEW_LOGGING
		LDAP_LOG( ACL, RESULTS, 
			   "acl_mask: [%d] applying %s (%s)\n",
			   i, accessmask2str( modmask, accessmaskbuf),
			   b->a_type == ACL_CONTINUE ? "continue" : b->a_type == ACL_BREAK
			   ? "break" : "stop"  );
#else
		Debug( LDAP_DEBUG_ACL,
			"<= acl_mask: [%d] applying %s (%s)\n",
			i, accessmask2str( modmask, accessmaskbuf ), 
			b->a_type == ACL_CONTINUE
				? "continue"
				: b->a_type == ACL_BREAK
					? "break"
					: "stop" );
#endif
		/* save old mask */
		oldmask = *mask;

		if( ACL_IS_ADDITIVE(modmask) ) {
			/* add privs */
			ACL_PRIV_SET( *mask, modmask );

			/* cleanup */
			ACL_PRIV_CLR( *mask, ~ACL_PRIV_MASK );

		} else if( ACL_IS_SUBTRACTIVE(modmask) ) {
			/* substract privs */
			ACL_PRIV_CLR( *mask, modmask );

			/* cleanup */
			ACL_PRIV_CLR( *mask, ~ACL_PRIV_MASK );

		} else {
			/* assign privs */
			*mask = modmask;
		}

#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			   "acl_mask: conn %lu  [%d] mask: %s\n",
			   conn->c_connid, i, accessmask2str( *mask, accessmaskbuf)  );
#else
		Debug( LDAP_DEBUG_ACL,
			"<= acl_mask: [%d] mask: %s\n",
			i, accessmask2str(*mask, accessmaskbuf), 0 );
#endif

		if( b->a_type == ACL_CONTINUE ) {
			continue;

		} else if ( b->a_type == ACL_BREAK ) {
			return ACL_BREAK;

		} else {
			return ACL_STOP;
		}
	}

	/* implicit "by * none" clause */
	ACL_INIT(*mask);

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, RESULTS, 
		   "acl_mask: conn %lu  no more <who> clauses, returning %d (stop)\n",
		   conn->c_connid, accessmask2str( *mask, accessmaskbuf) , 0 );
#else
	Debug( LDAP_DEBUG_ACL,
		"<= acl_mask: no more <who> clauses, returning %s (stop)\n",
		accessmask2str(*mask, accessmaskbuf), 0, 0 );
#endif
	return ACL_STOP;
}

/*
 * acl_check_modlist - check access control on the given entry to see if
 * it allows the given modifications by the user associated with op.
 * returns	1	if mods allowed ok
 *			0	mods not allowed
 */

int
acl_check_modlist(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    Entry	*e,
    Modifications	*mlist
)
{
	struct berval *bv;
	AccessControlState state = ACL_STATE_INIT;

	assert( be != NULL );

	/* short circuit root database access */
	if ( be_isroot( be, &op->o_ndn ) ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			   "acl_check_modlist: conn %lu  access granted to root user\n",
			   conn->c_connid, 0, 0 );
#else
		Debug( LDAP_DEBUG_ACL,
			"<= acl_access_allowed: granted to database root\n",
		    0, 0, 0 );
#endif
		return 1;
	}

	/* use backend default access if no backend acls */
	if( be != NULL && be->be_acl == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"acl_check_modlist: backend default %s access %s to \"%s\"\n",
			access2str( ACL_WRITE ),
			be->be_dfltaccess >= ACL_WRITE ? "granted" : "denied", 
			op->o_dn.bv_val  );
#else
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: backend default %s access %s to \"%s\"\n",
			access2str( ACL_WRITE ),
			be->be_dfltaccess >= ACL_WRITE ? "granted" : "denied", op->o_dn.bv_val );
#endif
		return be->be_dfltaccess >= ACL_WRITE;

#ifdef notdef
	/* be is always non-NULL */
	/* use global default access if no global acls */
	} else if ( be == NULL && global_acl == NULL ) {
#ifdef NEW_LOGGING
		LDAP_LOG( ACL, DETAIL1, 
			"acl_check_modlist: global default %s access %s to \"%s\"\n",
		   access2str( ACL_WRITE ),
		   global_default_access >= ACL_WRITE ? "granted" : "denied", 
		   op->o_dn  );
#else
		Debug( LDAP_DEBUG_ACL,
			"=> access_allowed: global default %s access %s to \"%s\"\n",
			access2str( ACL_WRITE ),
			global_default_access >= ACL_WRITE ? "granted" : "denied", op->o_dn );
#endif
		return global_default_access >= ACL_WRITE;
#endif
	}

	for ( ; mlist != NULL; mlist = mlist->sml_next ) {
		/*
		 * no-user-modification operational attributes are ignored
		 * by ACL_WRITE checking as any found here are not provided
		 * by the user
		 */
		if ( is_at_no_user_mod( mlist->sml_desc->ad_type ) ) {
#ifdef NEW_LOGGING
			LDAP_LOG( ACL, DETAIL1, 
				   "acl_check_modlist: conn %lu  no-user-mod %s: modify access granted\n",
				   conn->c_connid, mlist->sml_desc->ad_cname.bv_val , 0 );
#else
			Debug( LDAP_DEBUG_ACL, "acl: no-user-mod %s:"
				" modify access granted\n",
				mlist->sml_desc->ad_cname.bv_val, 0, 0 );
#endif
			continue;
		}

		switch ( mlist->sml_op ) {
		case LDAP_MOD_REPLACE:
			/*
			 * We must check both permission to delete the whole
			 * attribute and permission to add the specific attributes.
			 * This prevents abuse from selfwriters.
			 */
			if ( ! access_allowed( be, conn, op, e,
				mlist->sml_desc, NULL, ACL_WRITE, &state ) )
			{
				return( 0 );
			}

			if ( mlist->sml_bvalues == NULL ) break;

			/* fall thru to check value to add */

		case LDAP_MOD_ADD:
			assert( mlist->sml_bvalues != NULL );

			for ( bv = mlist->sml_bvalues; bv->bv_val != NULL; bv++ ) {
				if ( ! access_allowed( be, conn, op, e,
					mlist->sml_desc, bv, ACL_WRITE, &state ) )
				{
					return( 0 );
				}
			}
			break;

		case LDAP_MOD_DELETE:
			if ( mlist->sml_bvalues == NULL ) {
				if ( ! access_allowed( be, conn, op, e,
					mlist->sml_desc, NULL, ACL_WRITE, NULL ) )
				{
					return( 0 );
				}
				break;
			}
			for ( bv = mlist->sml_bvalues; bv->bv_val != NULL; bv++ ) {
				if ( ! access_allowed( be, conn, op, e,
					mlist->sml_desc, bv, ACL_WRITE, &state ) )
				{
					return( 0 );
				}
			}
			break;

		case SLAP_MOD_SOFTADD:
			/* allow adding attribute via modrdn thru */
			break;

		default:
			assert( 0 );
			return( 0 );
		}
	}

	return( 1 );
}

static int
aci_get_part(
	struct berval *list,
	int ix,
	char sep,
	struct berval *bv )
{
	int len;
	char *p;

	if (bv) {
		bv->bv_len = 0;
		bv->bv_val = NULL;
	}
	len = list->bv_len;
	p = list->bv_val;
	while (len >= 0 && --ix >= 0) {
		while (--len >= 0 && *p++ != sep) ;
	}
	while (len >= 0 && *p == ' ') {
		len--;
		p++;
	}
	if (len < 0)
		return(-1);

	if (!bv)
		return(0);

	bv->bv_val = p;
	while (--len >= 0 && *p != sep) {
		bv->bv_len++;
		p++;
	}
	while (bv->bv_len > 0 && *--p == ' ')
		bv->bv_len--;
	return(bv->bv_len);
}

BerVarray
aci_set_gather (void *cookie, struct berval *name, struct berval *attr)
{
	AciSetCookie *cp = cookie;
	BerVarray bvals = NULL;
	struct berval ndn;

	/* this routine needs to return the bervals instead of
	 * plain strings, since syntax is not known.  It should
	 * also return the syntax or some "comparison cookie".
	 */

	if (dnNormalize2(NULL, name, &ndn) == LDAP_SUCCESS) {
		const char *text;
		AttributeDescription *desc = NULL;
		if (slap_bv2ad(attr, &desc, &text) == LDAP_SUCCESS) {
			backend_attribute(cp->be, NULL, cp->op,
				cp->e, &ndn, desc, &bvals);
		}
		free(ndn.bv_val);
	}
	return(bvals);
}

static int
aci_match_set (
	struct berval *subj,
    Backend *be,
    Entry *e,
    Connection *conn,
    Operation *op,
    int setref
)
{
	struct berval set = { 0, NULL };
	int rc = 0;
	AciSetCookie cookie;

	if (setref == 0) {
		ber_dupbv( &set, subj );
	} else {
		struct berval subjdn, ndn = { 0, NULL };
		struct berval setat;
		BerVarray bvals;
		const char *text;
		AttributeDescription *desc = NULL;

		/* format of string is "entry/setAttrName" */
		if (aci_get_part(subj, 0, '/', &subjdn) < 0) {
			return(0);
		}

		if ( aci_get_part(subj, 1, '/', &setat) < 0 ) {
			setat.bv_val = SLAPD_ACI_SET_ATTR;
			setat.bv_len = sizeof(SLAPD_ACI_SET_ATTR)-1;
		}

		if ( setat.bv_val != NULL ) {
			/*
			 * NOTE: dnNormalize2 honors the ber_len field
			 * as the length of the dn to be normalized
			 */
			if ( dnNormalize2(NULL, &subjdn, &ndn) == LDAP_SUCCESS
				&& slap_bv2ad(&setat, &desc, &text) == LDAP_SUCCESS )
			{
				backend_attribute(be, NULL, op, e,
					&ndn, desc, &bvals);
				if ( bvals != NULL ) {
					if ( bvals[0].bv_val != NULL ) {
						int i;
						set = bvals[0];
						bvals[0].bv_val = NULL;
						for (i=1;bvals[i].bv_val;i++);
						bvals[0].bv_val = bvals[i-1].bv_val;
						bvals[i-1].bv_val = NULL;
					}
					ber_bvarray_free(bvals);
				}
			}
			if (ndn.bv_val)
				free(ndn.bv_val);
		}
	}

	if (set.bv_val != NULL) {
		cookie.be = be;
		cookie.e = e;
		cookie.conn = conn;
		cookie.op = op;
		rc = (slap_set_filter(aci_set_gather, &cookie, &set,
			&op->o_ndn, &e->e_nname, NULL) > 0);
		ch_free(set.bv_val);
	}
	return(rc);
}

#ifdef SLAPD_ACI_ENABLED
static int
aci_list_map_rights(
	struct berval *list )
{
	struct berval bv;
	slap_access_t mask;
	int i;

	ACL_INIT(mask);
	for (i = 0; aci_get_part(list, i, ',', &bv) >= 0; i++) {
		if (bv.bv_len <= 0)
			continue;
		switch (*bv.bv_val) {
		case 'c':
			ACL_PRIV_SET(mask, ACL_PRIV_COMPARE);
			break;
		case 's':
			/* **** NOTE: draft-ietf-ldapext-aci-model-0.3.txt defines
			 * the right 's' to mean "set", but in the examples states
			 * that the right 's' means "search".  The latter definition
			 * is used here.
			 */
			ACL_PRIV_SET(mask, ACL_PRIV_SEARCH);
			break;
		case 'r':
			ACL_PRIV_SET(mask, ACL_PRIV_READ);
			break;
		case 'w':
			ACL_PRIV_SET(mask, ACL_PRIV_WRITE);
			break;
		case 'x':
			/* **** NOTE: draft-ietf-ldapext-aci-model-0.3.txt does not 
			 * define any equivalent to the AUTH right, so I've just used
			 * 'x' for now.
			 */
			ACL_PRIV_SET(mask, ACL_PRIV_AUTH);
			break;
		default:
			break;
		}

	}
	return(mask);
}

static int
aci_list_has_attr(
	struct berval *list,
	const struct berval *attr,
	struct berval *val )
{
	struct berval bv, left, right;
	int i;

	for (i = 0; aci_get_part(list, i, ',', &bv) >= 0; i++) {
		if (aci_get_part(&bv, 0, '=', &left) < 0
			|| aci_get_part(&bv, 1, '=', &right) < 0)
		{
			if (ber_bvstrcasecmp(attr, &bv) == 0)
				return(1);
		} else if (val == NULL) {
			if (ber_bvstrcasecmp(attr, &left) == 0)
				return(1);
		} else {
			if (ber_bvstrcasecmp(attr, &left) == 0) {
				/* this is experimental code that implements a
				 * simple (prefix) match of the attribute value.
				 * the ACI draft does not provide for aci's that
				 * apply to specific values, but it would be
				 * nice to have.  If the <attr> part of an aci's
				 * rights list is of the form <attr>=<value>,
				 * that means the aci applies only to attrs with
				 * the given value.  Furthermore, if the attr is
				 * of the form <attr>=<value>*, then <value> is
				 * treated as a prefix, and the aci applies to 
				 * any value with that prefix.
				 *
				 * Ideally, this would allow r.e. matches.
				 */
				if (aci_get_part(&right, 0, '*', &left) < 0
					|| right.bv_len <= left.bv_len)
				{
					if (ber_bvstrcasecmp(val, &right) == 0)
						return(1);
				} else if (val->bv_len >= left.bv_len) {
					if (strncasecmp( val->bv_val, left.bv_val, left.bv_len ) == 0)
						return(1);
				}
			}
		}
	}
	return(0);
}

static slap_access_t
aci_list_get_attr_rights(
	struct berval *list,
	const struct berval *attr,
	struct berval *val )
{
    struct berval bv;
    slap_access_t mask;
    int i;

	/* loop through each rights/attr pair, skip first part (action) */
	ACL_INIT(mask);
	for (i = 1; aci_get_part(list, i + 1, ';', &bv) >= 0; i += 2) {
		if (aci_list_has_attr(&bv, attr, val) == 0)
			continue;
		if (aci_get_part(list, i, ';', &bv) < 0)
			continue;
		mask |= aci_list_map_rights(&bv);
	}
	return(mask);
}

static int
aci_list_get_rights(
	struct berval *list,
	const struct berval *attr,
	struct berval *val,
	slap_access_t *grant,
	slap_access_t *deny )
{
    struct berval perm, actn;
    slap_access_t *mask;
    int i, found;

	if (attr == NULL || attr->bv_len == 0 
			|| ber_bvstrcasecmp( attr, &aci_bv_entry ) == 0) {
		attr = &aci_bv_br_entry;
	}

	found = 0;
	ACL_INIT(*grant);
	ACL_INIT(*deny);
	/* loop through each permissions clause */
	for (i = 0; aci_get_part(list, i, '$', &perm) >= 0; i++) {
		if (aci_get_part(&perm, 0, ';', &actn) < 0)
			continue;
		if (ber_bvstrcasecmp( &aci_bv_grant, &actn ) == 0) {
			mask = grant;
		} else if (ber_bvstrcasecmp( &aci_bv_deny, &actn ) == 0) {
			mask = deny;
		} else {
			continue;
		}

		found = 1;
		*mask |= aci_list_get_attr_rights(&perm, attr, val);
		*mask |= aci_list_get_attr_rights(&perm, &aci_bv_br_all, NULL);
	}
	return(found);
}

static int
aci_group_member (
	struct berval *subj,
	struct berval *defgrpoc,
	struct berval *defgrpat,
    Backend		*be,
    Entry		*e,
    Connection		*conn,
    Operation		*op,
	regmatch_t	*matches
)
{
	struct berval subjdn;
	struct berval grpoc;
	struct berval grpat;
	ObjectClass *grp_oc = NULL;
	AttributeDescription *grp_ad = NULL;
	const char *text;
	int rc;

	/* format of string is "group/objectClassValue/groupAttrName" */
	if (aci_get_part(subj, 0, '/', &subjdn) < 0) {
		return(0);
	}

	if (aci_get_part(subj, 1, '/', &grpoc) < 0) {
		grpoc = *defgrpoc;
	}

	if (aci_get_part(subj, 2, '/', &grpat) < 0) {
		grpat = *defgrpat;
	}

	rc = slap_bv2ad( &grpat, &grp_ad, &text );
	if( rc != LDAP_SUCCESS ) {
		rc = 0;
		goto done;
	}
	rc = 0;

	grp_oc = oc_bvfind( &grpoc );

	if (grp_oc != NULL && grp_ad != NULL ) {
		char buf[ACL_BUF_SIZE];
		struct berval bv, ndn;
		bv.bv_len = sizeof( buf ) - 1;
		bv.bv_val = (char *)&buf;
		string_expand(&bv, &subjdn, e->e_ndn, matches);
		if ( dnNormalize2(NULL, &bv, &ndn) == LDAP_SUCCESS ) {
			rc = (backend_group(be, conn, op, e, &ndn, &op->o_ndn,
				grp_oc, grp_ad) == 0);
			free( ndn.bv_val );
		}
	}

done:
	return(rc);
}

static int
aci_mask(
    Backend			*be,
    Connection		*conn,
    Operation		*op,
    Entry			*e,
	AttributeDescription *desc,
    struct berval	*val,
    struct berval	*aci,
	regmatch_t		*matches,
	slap_access_t	*grant,
	slap_access_t	*deny
)
{
    struct berval bv, perms, sdn;
	int rc;
		

	assert( desc->ad_cname.bv_val != NULL );

	/* parse an aci of the form:
		oid#scope#action;rights;attr;rights;attr$action;rights;attr;rights;attr#dnType#subjectDN

	   See draft-ietf-ldapext-aci-model-04.txt section 9.1 for
	   a full description of the format for this attribute.
	   Differences: "this" in the draft is "self" here, and
	   "self" and "public" is in the position of dnType.

	   For now, this routine only supports scope=entry.
	 */

	/* check that the aci has all 5 components */
	if (aci_get_part(aci, 4, '#', NULL) < 0)
		return(0);

	/* check that the aci family is supported */
	if (aci_get_part(aci, 0, '#', &bv) < 0)
		return(0);

	/* check that the scope is "entry" */
	if (aci_get_part(aci, 1, '#', &bv) < 0
		|| ber_bvstrcasecmp( &aci_bv_entry, &bv ) != 0)
	{
		return(0);
	}

	/* get the list of permissions clauses, bail if empty */
	if (aci_get_part(aci, 2, '#', &perms) <= 0)
		return(0);

	/* check if any permissions allow desired access */
	if (aci_list_get_rights(&perms, &desc->ad_cname, val, grant, deny) == 0)
		return(0);

	/* see if we have a DN match */
	if (aci_get_part(aci, 3, '#', &bv) < 0)
		return(0);

	if (aci_get_part(aci, 4, '#', &sdn) < 0)
		return(0);

	if (ber_bvstrcasecmp( &aci_bv_access_id, &bv ) == 0) {
		struct berval ndn;
		rc = 0;
		if ( dnNormalize2(NULL, &sdn, &ndn) == LDAP_SUCCESS ) {
			if (dn_match( &op->o_ndn, &ndn))
				rc = 1;
			free(ndn.bv_val);
		}
		return (rc);

	} else if (ber_bvstrcasecmp( &aci_bv_public, &bv ) == 0) {
		return(1);

	} else if (ber_bvstrcasecmp( &aci_bv_self, &bv ) == 0) {
		if (dn_match(&op->o_ndn, &e->e_nname))
			return(1);

	} else if (ber_bvstrcasecmp( &aci_bv_dnattr, &bv ) == 0) {
		Attribute *at;
		AttributeDescription *ad = NULL;
		const char *text;

		rc = slap_bv2ad( &sdn, &ad, &text );

		if( rc != LDAP_SUCCESS ) {
			return 0;
		}

		rc = 0;

		bv = op->o_ndn;

		for(at = attrs_find( e->e_attrs, ad );
			at != NULL;
			at = attrs_find( at->a_next, ad ) )
		{
			if (value_find_ex( ad, SLAP_MR_VALUE_NORMALIZED_MATCH, at->a_vals, &bv) == 0 ) {
				rc = 1;
				break;
			}
		}

		return rc;


	} else if (ber_bvstrcasecmp( &aci_bv_group, &bv ) == 0) {
		if (aci_group_member(&sdn, &aci_bv_group_class, &aci_bv_group_attr, be, e, conn, op, matches))
			return(1);

	} else if (ber_bvstrcasecmp( &aci_bv_role, &bv ) == 0) {
		if (aci_group_member(&sdn, &aci_bv_role_class, &aci_bv_role_attr, be, e, conn, op, matches))
			return(1);

	} else if (ber_bvstrcasecmp( &aci_bv_set, &bv ) == 0) {
		if (aci_match_set(&sdn, be, e, conn, op, 0))
			return(1);

	} else if (ber_bvstrcasecmp( &aci_bv_set_ref, &bv ) == 0) {
		if (aci_match_set(&sdn, be, e, conn, op, 1))
			return(1);

	}

	return(0);
}

#endif	/* SLAPD_ACI_ENABLED */

static void
string_expand(
	struct berval *bv,
	struct berval *pat,
	char *match,
	regmatch_t *matches)
{
	ber_len_t	size;
	char   *sp;
	char   *dp;
	int	flag;

	size = 0;
	bv->bv_val[0] = '\0';
	bv->bv_len--; /* leave space for lone $ */

	flag = 0;
	for ( dp = bv->bv_val, sp = pat->bv_val; size < bv->bv_len &&
		sp < pat->bv_val + pat->bv_len ; sp++ )
	{
		/* did we previously see a $ */
		if ( flag ) {
			if ( flag == 1 && *sp == '$' ) {
				*dp++ = '$';
				size++;
				flag = 0;

			} else if ( flag == 1 && *sp == '{' /*'}'*/) {
				flag = 2;

			} else if ( *sp >= '0' && *sp <= '9' ) {
				int	n;
				int	i;
				int	l;

				n = *sp - '0';

				if ( flag == 2 ) {
					for ( sp++; *sp != '\0' && *sp != /*'{'*/ '}'; sp++ ) {
						if ( *sp >= '0' && *sp <= '9' ) {
							n = 10*n + ( *sp - '0' );
						}
					}

					if ( *sp != /*'{'*/ '}' ) {
						/* error */
					}
				}

				if ( n >= MAXREMATCHES ) {
				
				}
				
				*dp = '\0';
				i = matches[n].rm_so;
				l = matches[n].rm_eo; 
				for ( ; size < bv->bv_len && i < l; size++, i++ ) {
					*dp++ = match[i];
				}
				*dp = '\0';

				flag = 0;
			}
		} else {
			if (*sp == '$') {
				flag = 1;
			} else {
				*dp++ = *sp;
				size++;
			}
		}
	}

	if ( flag ) {
		/* must have ended with a single $ */
		*dp++ = '$';
		size++;
	}

	*dp = '\0';
	bv->bv_len = size;

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, DETAIL1, 
	   "string_expand:  pattern = %.*s\n", (int)pat->bv_len, pat->bv_val, 0 );
	LDAP_LOG( ACL, DETAIL1, "string_expand:  expanded = %s\n", bv->bv_val, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "=> string_expand: pattern:  %.*s\n", (int)pat->bv_len, pat->bv_val, 0 );
	Debug( LDAP_DEBUG_TRACE, "=> string_expand: expanded: %s\n", bv->bv_val, 0, 0 );
#endif
}

static int
regex_matches(
	struct berval *pat,			/* pattern to expand and match against */
	char *str,				/* string to match against pattern */
	char *buf,				/* buffer with $N expansion variables */
	regmatch_t *matches		/* offsets in buffer for $N expansion variables */
)
{
	regex_t re;
	char newbuf[ACL_BUF_SIZE];
	struct berval bv;
	int	rc;

	bv.bv_len = sizeof(newbuf) - 1;
	bv.bv_val = newbuf;

	if(str == NULL) str = "";

	string_expand(&bv, pat, buf, matches);
	if (( rc = regcomp(&re, newbuf, REG_EXTENDED|REG_ICASE))) {
		char error[ACL_BUF_SIZE];
		regerror(rc, &re, error, sizeof(error));

#ifdef NEW_LOGGING
		LDAP_LOG( ACL, ERR, 
			   "regex_matches: compile( \"%s\", \"%s\") failed %s\n",
			   pat->bv_val, str, error  );
#else
		Debug( LDAP_DEBUG_TRACE,
		    "compile( \"%s\", \"%s\") failed %s\n",
			pat->bv_val, str, error );
#endif
		return( 0 );
	}

	rc = regexec(&re, str, 0, NULL, 0);
	regfree( &re );

#ifdef NEW_LOGGING
	LDAP_LOG( ACL, DETAIL2, "regex_matches: string:   %s\n", str, 0, 0 );
	LDAP_LOG( ACL, DETAIL2, "regex_matches: rc:	%d  %s\n",
		   rc, rc ? "matches" : "no matches", 0  );
#else
	Debug( LDAP_DEBUG_TRACE,
	    "=> regex_matches: string:	 %s\n", str, 0, 0 );
	Debug( LDAP_DEBUG_TRACE,
	    "=> regex_matches: rc: %d %s\n",
		rc, !rc ? "matches" : "no matches", 0 );
#endif
	return( !rc );
}

