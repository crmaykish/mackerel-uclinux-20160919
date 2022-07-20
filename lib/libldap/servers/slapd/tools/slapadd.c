/* $OpenLDAP: pkg/ldap/servers/slapd/tools/slapadd.c,v 1.40.2.11 2003/03/03 17:10:11 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lber.h>
#include <ldif.h>
#include <lutil.h>

#include "slapcommon.h"

int
main( int argc, char **argv )
{
	char		*buf = NULL;
	int         lineno;
	int         lmax;
	int			rc = EXIT_SUCCESS;

	const char *text;
	char textbuf[SLAP_TEXT_BUFLEN] = { '\0' };
	size_t textlen = sizeof textbuf;

#ifdef NEW_LOGGING
	lutil_log_initialize(argc, argv );
#endif
	slap_tool_init( "slapadd", SLAPADD, argc, argv );

	if( !be->be_entry_open ||
		!be->be_entry_close ||
		!be->be_entry_put )
	{
		fprintf( stderr, "%s: database doesn't support necessary operations.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	lmax = 0;
	lineno = 0;

	if( be->be_entry_open( be, 1 ) != 0 ) {
		fprintf( stderr, "%s: could not open database.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	while( ldif_read_record( ldiffp, &lineno, &buf, &lmax ) ) {
		Entry *e = str2entry( buf );
		struct berval bvtext;

		/*
		 * Initialize text buffer
		 */
		bvtext.bv_len = textlen;
		bvtext.bv_val = textbuf;
		bvtext.bv_val[0] = '\0';

		if( e == NULL ) {
			fprintf( stderr, "%s: could not parse entry (line=%d)\n",
				progname, lineno );
			rc = EXIT_FAILURE;
			if( continuemode ) continue;
			break;
		}

		/* make sure the DN is not empty */
		if( !e->e_nname.bv_len ) {
			fprintf( stderr, "%s: empty dn=\"%s\" (line=%d)\n",
				progname, e->e_dn, lineno );
			rc = EXIT_FAILURE;
			entry_free( e );
			if( continuemode ) continue;
			break;
		}

		/* check backend */
		if( select_backend( &e->e_nname, is_entry_referral(e), nosubordinates )
			!= be )
		{
			fprintf( stderr, "%s: line %d: "
				"database (%s) not configured to hold \"%s\"\n",
				progname, lineno,
				be ? be->be_suffix[0].bv_val : "<none>",
				e->e_dn );
			fprintf( stderr, "%s: line %d: "
				"database (%s) not configured to hold \"%s\"\n",
				progname, lineno,
				be ? be->be_nsuffix[0].bv_val : "<none>",
				e->e_ndn );
			rc = EXIT_FAILURE;
			entry_free( e );
			if( continuemode ) continue;
			break;
		}

		if( global_schemacheck ) {
			Attribute *sc = attr_find( e->e_attrs,
				slap_schema.si_ad_structuralObjectClass );
			Attribute *oc = attr_find( e->e_attrs,
				slap_schema.si_ad_objectClass );

			if( oc == NULL ) {
				fprintf( stderr, "%s: dn=\"%s\" (line=%d): %s\n",
					progname, e->e_dn, lineno,
					"no objectClass attribute");
				rc = EXIT_FAILURE;
				entry_free( e );
				if( continuemode ) continue;
				break;
			}

			if( sc == NULL ) {
				struct berval vals[2];

				rc = structural_class( oc->a_vals, vals,
					NULL, &text, textbuf, textlen );

				if( rc != LDAP_SUCCESS ) {
					fprintf( stderr, "%s: dn=\"%s\" (line=%d): (%d) %s\n",
						progname, e->e_dn, lineno, rc, text );
					rc = EXIT_FAILURE;
					entry_free( e );
					if( continuemode ) continue;
					break;
				}

				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_structuralObjectClass,
					vals );
			}

			/* check schema */
			rc = entry_schema_check( be, e, NULL, &text, textbuf, textlen );

			if( rc != LDAP_SUCCESS ) {
				fprintf( stderr, "%s: dn=\"%s\" (line=%d): (%d) %s\n",
					progname, e->e_dn, lineno, rc, text );
				rc = EXIT_FAILURE;
				entry_free( e );
				if( continuemode ) continue;
				break;
			}
		}

		if ( SLAP_LASTMOD(be) ) {
			struct tm *ltm;
			time_t now = slap_get_time();
			char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
			struct berval vals[ 2 ];

			struct berval name, timestamp, csn;
			char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
			char csnbuf[ LDAP_LUTIL_CSNSTR_BUFSIZE ];

			ltm = gmtime(&now);
			lutil_gentime( timebuf, sizeof(timebuf), ltm );

			csn.bv_len = lutil_csnstr( csnbuf, sizeof( csnbuf ), 0, 0 );
			csn.bv_val = csnbuf;

			timestamp.bv_val = timebuf;
			timestamp.bv_len = strlen(timebuf);

			if ( be->be_rootndn.bv_len == 0 ) {
				name.bv_val = SLAPD_ANONYMOUS;
				name.bv_len = sizeof(SLAPD_ANONYMOUS) - 1;
			} else {
				name = be->be_rootndn;
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_entryUUID )
				== NULL )
			{
				vals[0].bv_len = lutil_uuidstr( uuidbuf, sizeof( uuidbuf ) );
				vals[0].bv_val = uuidbuf;
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_entryUUID, vals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_creatorsName )
				== NULL )
			{
				vals[0] = name;
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_creatorsName, vals);
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_modifiersName )
				== NULL )
			{
				vals[0] = name;
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_modifiersName, vals);
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_createTimestamp )
				== NULL )
			{
				vals[0] = timestamp;
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_createTimestamp, vals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_modifyTimestamp )
				== NULL )
			{
				vals[0] = timestamp;
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_modifyTimestamp, vals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_entryCSN )
				== NULL )
			{
				vals[0] = csn;
				vals[1].bv_len = 0;
				vals[1].bv_val = NULL;
				attr_merge( e, slap_schema.si_ad_entryCSN, vals );
			}
		}

		if (!dryrun) {
			ID id = be->be_entry_put( be, e, &bvtext );
			if( id == NOID ) {
				fprintf( stderr, "%s: could not add entry dn=\"%s\" (line=%d): %s\n",
					progname, e->e_dn, lineno, bvtext.bv_val );
				rc = EXIT_FAILURE;
				entry_free( e );
				if( continuemode ) continue;
				break;
			}
		
			if ( verbose ) {
				fprintf( stderr, "added: \"%s\" (%08lx)\n",
					e->e_dn, (long) id );
			}
		} else {
			if ( verbose ) {
				fprintf( stderr, "(dry) added: \"%s\"\n", e->e_dn );
			}
		}

		entry_free( e );
	}

	ch_free( buf );

	if( be->be_entry_close( be )) rc = EXIT_FAILURE;

	if( be->be_sync ) {
		be->be_sync( be );
	}

	slap_tool_destroy();
	return rc;
}
