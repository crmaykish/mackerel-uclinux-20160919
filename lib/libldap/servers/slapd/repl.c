/* repl.c - log modifications for replication purposes */
/* $OpenLDAP: pkg/ldap/servers/slapd/repl.c,v 1.40.2.4 2003/03/03 17:10:07 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/socket.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "slap.h"
#include "ldif.h"

int
add_replica_info(
    Backend     *be,
    const char  *host 
)
{
	int i = 0;

	assert( be );
	assert( host );

	if ( be->be_replica != NULL ) {
		for ( ; be->be_replica[ i ] != NULL; i++ );
	}
		
	be->be_replica = ch_realloc( be->be_replica, 
		sizeof( struct slap_replica_info * )*( i + 2 ) );

	be->be_replica[ i ] 
		= ch_calloc( sizeof( struct slap_replica_info ), 1 );
	be->be_replica[ i ]->ri_host = ch_strdup( host );
	be->be_replica[ i ]->ri_nsuffix = NULL;
	be->be_replica[ i ]->ri_attrs = NULL;
	be->be_replica[ i + 1 ] = NULL;

	return( i );
}

int
add_replica_suffix(
    Backend     *be,
    int		nr,
    const char  *suffix
)
{
	struct berval dn, ndn;
	int rc;

	dn.bv_val = (char *) suffix;
	dn.bv_len = strlen( dn.bv_val );

	rc = dnNormalize2( NULL, &dn, &ndn );
	if( rc != LDAP_SUCCESS ) {
		return 2;
	}

	if ( select_backend( &ndn, 0, 0 ) != be ) {
		free( ndn.bv_val );
		return 1;
	}

	ber_bvarray_add( &be->be_replica[nr]->ri_nsuffix, &ndn );
	return 0;
}

int
add_replica_attrs(
	Backend	*be,
	int	nr,
	char	*attrs,
	int	exclude
)
{
	if ( be->be_replica[nr]->ri_attrs != NULL ) {
		if ( be->be_replica[nr]->ri_exclude != exclude ) {
			fprintf( stderr, "attr selective replication directive '%s' conflicts with previous one (discarded)\n", attrs );
			ch_free( be->be_replica[nr]->ri_attrs );
			be->be_replica[nr]->ri_attrs = NULL;
		}
	}

	be->be_replica[nr]->ri_exclude = exclude;
	be->be_replica[nr]->ri_attrs = str2anlist( be->be_replica[nr]->ri_attrs,
		attrs, "," );
	return ( be->be_replica[nr]->ri_attrs == NULL );
}
   
static void
print_vals( FILE *fp, struct berval *type, struct berval *bv );
static void
replog1( struct slap_replica_info *ri, Operation *op, void *change, FILE *fp, void *first);

void
replog(
    Backend	*be,
    Operation *op,
    struct berval *dn,
    struct berval *ndn,
    void	*change
)
{
	Modifications	*ml = NULL;
	Attribute	*a = NULL;
	Entry	*e;
	FILE	*fp, *lfp;
	int	i;
/* undef NO_LOG_WHEN_NO_REPLICAS */
#ifdef NO_LOG_WHEN_NO_REPLICAS
	int     count = 0;
#endif
	int	subsets = 0;
	long now = slap_get_time();

	if ( be->be_replogfile == NULL && replogfile == NULL ) {
		return;
	}

	ldap_pvt_thread_mutex_lock( &replog_mutex );
	if ( (fp = lock_fopen( be->be_replogfile ? be->be_replogfile :
	    replogfile, "a", &lfp )) == NULL ) {
		ldap_pvt_thread_mutex_unlock( &replog_mutex );
		return;
	}

	for ( i = 0; be->be_replica != NULL && be->be_replica[i] != NULL; i++ ) {
		/* check if dn's suffix matches legal suffixes, if any */
		if ( be->be_replica[i]->ri_nsuffix != NULL ) {
			int j;

			for ( j = 0; be->be_replica[i]->ri_nsuffix[j].bv_val; j++ ) {
				if ( dnIsSuffix( ndn, &be->be_replica[i]->ri_nsuffix[j] ) ) {
					break;
				}
			}

			if ( !be->be_replica[i]->ri_nsuffix[j].bv_val ) {
				/* do not add "replica:" line */
				continue;
			}
		}
		/* See if we only want a subset of attributes */
		if ( be->be_replica[i]->ri_attrs != NULL &&
			( op->o_tag == LDAP_REQ_MODIFY || op->o_tag == LDAP_REQ_ADD || op->o_tag == LDAP_REQ_EXTENDED ) ) {
			if ( !subsets ) {
				subsets = i + 1;
			}
			/* Do attribute subsets by themselves in a second pass */
			continue;
		}

		fprintf( fp, "replica: %s\n", be->be_replica[i]->ri_host );
#ifdef NO_LOG_WHEN_NO_REPLICAS
		++count;
#endif
	}

#ifdef NO_LOG_WHEN_NO_REPLICAS
	if ( count == 0 && subsets == 0 ) {
		/* if no replicas matched, drop the log 
		 * (should we log it anyway?) */
		lock_fclose( fp, lfp );
		ldap_pvt_thread_mutex_unlock( &replog_mutex );

		return;
	}
#endif

	fprintf( fp, "time: %ld\n", now );
	fprintf( fp, "dn: %s\n", dn->bv_val );

	replog1( NULL, op, change, fp, NULL );

	if ( subsets > 0 ) {
		void *first;
		for ( i = subsets - 1; be->be_replica != NULL && be->be_replica[i] != NULL; i++ ) {

			/* If no attrs, we already did this above */
			if ( be->be_replica[i]->ri_attrs == NULL ) {
				continue;
			}

			/* check if dn's suffix matches legal suffixes, if any */
			if ( be->be_replica[i]->ri_nsuffix != NULL ) {
				int j;

				for ( j = 0; be->be_replica[i]->ri_nsuffix[j].bv_val; j++ ) {
					if ( dnIsSuffix( ndn, &be->be_replica[i]->ri_nsuffix[j] ) ) {
						break;
					}
				}

				if ( !be->be_replica[i]->ri_nsuffix[j].bv_val ) {
					/* do not add "replica:" line */
					continue;
				}
			}
			subsets = 0;
			first = NULL;
			switch( op->o_tag ) {
			case LDAP_REQ_EXTENDED:
				/* quick hack for extended operations */
				/* assume change parameter is a Modfications* */
				/* fall thru */
			case LDAP_REQ_MODIFY:
				for ( ml = change; ml != NULL; ml = ml->sml_next ) {
					int is_in, exclude;

   					is_in = ad_inlist( ml->sml_desc, be->be_replica[i]->ri_attrs );
					exclude = be->be_replica[i]->ri_exclude;
					
					/*
					 * there might be a more clever way to do this test,
					 * but this way, at least, is comprehensible :)
					 */
					if ( ( is_in && !exclude ) || ( !is_in && exclude ) ) {
						subsets = 1;
						first = ml;
						break;
					}
				}
				if ( !subsets ) {
					continue;
				}
				break;
			case LDAP_REQ_ADD:
				e = change;
				for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
					int is_in, exclude;

   					is_in = ad_inlist( a->a_desc, be->be_replica[i]->ri_attrs );
					exclude = be->be_replica[i]->ri_exclude;
					
					if ( ( is_in && !exclude ) || ( !is_in && exclude ) ) {
						subsets = 1;
						first = a;
						break;
					}
				}
				if ( !subsets ) {
					continue;
				}
				break;
			default:
				/* Other operations were logged in the first pass */
				continue;
			}
			fprintf( fp, "replica: %s\n", be->be_replica[i]->ri_host );
			fprintf( fp, "time: %ld\n", now );
			fprintf( fp, "dn: %s\n", dn->bv_val );
			replog1( be->be_replica[i], op, change, fp, first );
		}
	}

	lock_fclose( fp, lfp );
	ldap_pvt_thread_mutex_unlock( &replog_mutex );
}


static void
replog1(
    struct slap_replica_info *ri,
    Operation *op,
    void	*change,
    FILE	*fp,
	void	*first
)
{
	Modifications	*ml;
	Attribute	*a;
	Entry		*e;
	struct slap_replog_moddn *moddn;

	switch ( op->o_tag ) {
	case LDAP_REQ_EXTENDED:
		/* quick hack for extended operations */
		/* assume change parameter is a Modfications* */
		/* fall thru */

	case LDAP_REQ_MODIFY:
		fprintf( fp, "changetype: modify\n" );
		ml = first ? first : change;
		for ( ; ml != NULL; ml = ml->sml_next ) {
			char *type;
			if ( ri && ri->ri_attrs ) {
				int is_in = ad_inlist( ml->sml_desc, ri->ri_attrs );

				if ( ( !is_in && !ri->ri_exclude ) || ( is_in && ri->ri_exclude ) ) {
					continue;
				}
			}
			type = ml->sml_desc->ad_cname.bv_val;
			switch ( ml->sml_op ) {
			case LDAP_MOD_ADD:
				fprintf( fp, "add: %s\n", type );
				break;

			case LDAP_MOD_DELETE:
				fprintf( fp, "delete: %s\n", type );
				break;

			case LDAP_MOD_REPLACE:
				fprintf( fp, "replace: %s\n", type );
				break;
			}
			if ( ml->sml_bvalues )
				print_vals( fp, &ml->sml_desc->ad_cname, ml->sml_bvalues );
			fprintf( fp, "-\n" );
		}
		break;

	case LDAP_REQ_ADD:
		e = change;
		fprintf( fp, "changetype: add\n" );
		a = first ? first : e->e_attrs;
		for ( ; a != NULL; a=a->a_next ) {
			if ( ri && ri->ri_attrs ) {
				int is_in = ad_inlist( a->a_desc, ri->ri_attrs );
				if ( ( !is_in && !ri->ri_exclude ) || ( is_in && ri->ri_exclude ) ) {
					continue;
				}
				/* If the list includes objectClass names,
				 * only include those classes in the
				 * objectClass attribute
				 */
				if ( a->a_desc == slap_schema.si_ad_objectClass ) {
					int ocs = 0;
					AttributeName *an;
					struct berval vals[2];
					vals[1].bv_val = NULL;
					vals[1].bv_len = 0;
					for ( an = ri->ri_attrs; an->an_name.bv_val; an++ ) {
						if ( an->an_oc ) {
							int i;
							for ( i=0; a->a_vals[i].bv_val; i++ ) {
								if ( a->a_vals[i].bv_len == an->an_name.bv_len
									&& !strcasecmp(a->a_vals[i].bv_val,
										an->an_name.bv_val ) ) {
									ocs = 1;
									vals[0] = an->an_name;
									print_vals( fp, &a->a_desc->ad_cname, vals );
									break;
								}
							}
						}
					}
					if ( ocs ) continue;
				}
			}
			print_vals( fp, &a->a_desc->ad_cname, a->a_vals );
		}
		break;

	case LDAP_REQ_DELETE:
		fprintf( fp, "changetype: delete\n" );
		break;

	case LDAP_REQ_MODRDN:
		moddn = change;
		fprintf( fp, "changetype: modrdn\n" );
		fprintf( fp, "newrdn: %s\n", moddn->newrdn->bv_val );
		fprintf( fp, "deleteoldrdn: %d\n", moddn->deloldrdn ? 1 : 0 );
		/* moddn->newsup is never NULL, see modrdn.c */
		if( moddn->newsup->bv_val != NULL ) {
			fprintf( fp, "newsuperior: %s\n", moddn->newsup->bv_val );
		}
	}
	fprintf( fp, "\n" );
}

static void
print_vals(
	FILE *fp,
	struct berval *type,
	struct berval *bv )
{
	ber_len_t i, len;
	char	*buf, *bufp;

	for ( i = 0, len = 0; bv && bv[i].bv_val; i++ ) {
		if ( bv[i].bv_len > len )
			len = bv[i].bv_len;
	}

	len = LDIF_SIZE_NEEDED( type->bv_len, len ) + 1;
	buf = (char *) ch_malloc( len );

	for ( ; bv && bv->bv_val; bv++ ) {
		bufp = buf;
		ldif_sput( &bufp, LDIF_PUT_VALUE, type->bv_val,
				    bv->bv_val, bv->bv_len );
		*bufp = '\0';

		fputs( buf, fp );

	}
	free( buf );
}
