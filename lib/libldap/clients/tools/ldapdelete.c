/* ldapdelete.c - simple program to delete an entry using LDAP */
/* $OpenLDAP: pkg/ldap/clients/tools/ldapdelete.c,v 1.80.2.9 2003/03/29 15:45:43 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include <ldap.h>
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"

#include "common.h"


static int	prune = 0;


static int dodelete LDAP_P((
    LDAP *ld,
    const char *dn));

static int deletechildren LDAP_P((
	LDAP *ld,
	const char *dn ));

void
usage( void )
{
	fprintf( stderr,
"Delete entries from an LDAP server\n\n"
"usage: %s [options] [dn]...\n"
"	dn: list of DNs to delete. If not given, it will be readed from stdin\n"
"	    or from the file specified with \"-f file\".\n"
"Delete Options:\n"
"  -r         delete recursively\n"
			 , prog );
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "r"
	"cCd:D:e:f:h:H:IkKMnO:p:P:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	switch ( i ) {
#if 0
		int crit;
		char *control, *cvalue;
	case 'E': /* delete controls */
		if( protocol == LDAP_VERSION2 ) {
			fprintf( stderr, "%s: -E incompatible with LDAPv%d\n",
				prog, protocol );
			exit( EXIT_FAILURE );
		}

		/* should be extended to support comma separated list of
		 *	[!]key[=value] parameters, e.g.  -E !foo,bar=567
		 */

		crit = 0;
		cvalue = NULL;
		if( optarg[0] == '!' ) {
			crit = 1;
			optarg++;
		}

		control = strdup( optarg );
		if ( (cvalue = strchr( control, '=' )) != NULL ) {
			*cvalue++ = '\0';
		}
		fprintf( stderr, "Invalid delete control name: %s\n", control );
		usage();
#endif

	case 'r':
		prune = 1;
		break;

	default:
		return 0;
	}
	return 1;
}


static void
private_conn_setup( LDAP *ld )
{
	/* this seems prudent for searches below */
	int deref = LDAP_DEREF_NEVER;
	ldap_set_option( ld, LDAP_OPT_DEREF, &deref );
}


int
main( int argc, char **argv )
{
	char		buf[ 4096 ];
	FILE		*fp;
	LDAP		*ld;
	int		rc, retval;

    fp = NULL;

    prog = lutil_progname( "ldapdelete", argc, argv );

	tool_args( argc, argv );

	if ( infile != NULL ) {
		if (( fp = fopen( infile, "r" )) == NULL ) {
			perror( optarg );
			exit( EXIT_FAILURE );
	    }
	} else {
	if ( optind >= argc ) {
	    fp = stdin;
	}
    }

	ld = tool_conn_setup( 0, &private_conn_setup );

	if ( pw_file || want_bindpw ) {
		if ( pw_file ) {
			rc = lutil_get_filed_password( pw_file, &passwd );
			if( rc ) return EXIT_FAILURE;
		} else {
			passwd.bv_val = getpassphrase( "Enter LDAP Password: " );
			passwd.bv_len = passwd.bv_val ? strlen( passwd.bv_val ) : 0;
		}
	}

	tool_bind( ld );

	if ( authzid || manageDSAit || noop )
		tool_server_controls( ld, NULL, 0 );

	retval = rc = 0;

    if ( fp == NULL ) {
		for ( ; optind < argc; ++optind ) {
			rc = dodelete( ld, argv[ optind ] );

			/* Stop on error and no -c option */
			if( rc != 0 ) {
				retval = rc;
				if( contoper == 0 ) break;
			}
		}
	} else {
		while ((rc == 0 || contoper) && fgets(buf, sizeof(buf), fp) != NULL) {
			buf[ strlen( buf ) - 1 ] = '\0'; /* remove trailing newline */

			if ( *buf != '\0' ) {
				rc = dodelete( ld, buf );
				if ( rc != 0 )
					retval = rc;
			}
		}
	}

    ldap_unbind( ld );

    return( retval );
}


static int dodelete(
    LDAP	*ld,
    const char	*dn)
{
	int id;
	int	rc, code;
	char *matcheddn = NULL, *text = NULL, **refs = NULL;
	LDAPMessage *res;

	if ( verbose ) {
		printf( "%sdeleting entry \"%s\"\n",
			(not ? "!" : ""), dn );
	}

	if ( not ) {
		return LDAP_SUCCESS;
	}

	/* If prune is on, remove a whole subtree.  Delete the children of the
	 * DN recursively, then the DN requested.
	 */
	if ( prune ) deletechildren( ld, dn );

	rc = ldap_delete_ext( ld, dn, NULL, NULL, &id );
	if ( rc != LDAP_SUCCESS ) {
		fprintf( stderr, "%s: ldap_delete_ext: %s (%d)\n",
			prog, ldap_err2string( rc ), rc );
		return rc;
	}

	rc = ldap_result( ld, LDAP_RES_ANY, LDAP_MSG_ALL, NULL, &res );
	if ( rc < 0 ) {
		ldap_perror( ld, "ldapdelete: ldap_result" );
		return rc;
	}

	rc = ldap_parse_result( ld, res, &code, &matcheddn, &text, &refs, NULL, 1 );

	if( rc != LDAP_SUCCESS ) {
		fprintf( stderr, "%s: ldap_parse_result: %s (%d)\n",
			prog, ldap_err2string( rc ), rc );
		return rc;
	}

	if( verbose || code != LDAP_SUCCESS ||
		(matcheddn && *matcheddn) || (text && *text) || (refs && *refs) )
	{
		printf( "Delete Result: %s (%d)\n", ldap_err2string( code ), code );

		if( text && *text ) {
			printf( "Additional info: %s\n", text );
		}

		if( matcheddn && *matcheddn ) {
			printf( "Matched DN: %s\n", matcheddn );
		}

		if( refs ) {
			int i;
			for( i=0; refs[i]; i++ ) {
				printf("Referral: %s\n", refs[i] );
			}
		}
	}

	ber_memfree( text );
	ber_memfree( matcheddn );
	ber_memvfree( (void **) refs );

	return code;
}

/*
 * Delete all the children of an entry recursively until leaf nodes are reached.
 *
 */
static int deletechildren(
	LDAP *ld,
	const char *dn )
{
	LDAPMessage *res, *e;
	int entries;
	int rc;
	static char *attrs[] = { "1.1", NULL };

	if ( verbose ) printf ( "deleting children of: %s\n", dn );
	/*
	 * Do a one level search at dn for children.  For each, delete its children.
	 */

	rc = ldap_search_ext_s( ld, dn, LDAP_SCOPE_ONELEVEL, NULL, attrs, 1,
		NULL, NULL, NULL, -1, &res );
	if ( rc != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_search" );
		return( rc );
	}

	entries = ldap_count_entries( ld, res );

	if ( entries > 0 ) {
		int i;

		for (e = ldap_first_entry( ld, res ), i = 0; e != NULL;
			e = ldap_next_entry( ld, e ), i++ )
		{
			char *dn = ldap_get_dn( ld, e );

			if( dn == NULL ) {
				ldap_perror( ld, "ldap_prune" );
				ldap_get_option( ld, LDAP_OPT_ERROR_NUMBER, &rc );
				ber_memfree( dn );
				return rc;
			}

			rc = deletechildren( ld, dn );
			if ( rc == -1 ) {
				ldap_perror( ld, "ldap_prune" );
				ber_memfree( dn );
				return rc;
			}

			if ( verbose ) {
				printf( "\tremoving %s\n", dn );
			}

			rc = ldap_delete_s( ld, dn );
			if ( rc == -1 ) {
				ldap_perror( ld, "ldap_delete" );
				ber_memfree( dn );
				return rc;

			}
			
			if ( verbose ) {
				printf( "\t%s removed\n", dn );
			}

			ber_memfree( dn );
		}
	}

	ldap_msgfree( res );
	return rc;
}
