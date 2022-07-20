/* $OpenLDAP: pkg/ldap/clients/tools/ldappasswd.c,v 1.94.2.10 2003/04/14 15:37:27 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <ldap.h>
#include "lutil.h"
#include "lutil_ldap.h"
#include "ldap_defaults.h"

#include "common.h"


static struct berval newpw = { 0, NULL };
static struct berval oldpw = { 0, NULL };

static int   want_newpw = 0;
static int   want_oldpw = 0;

static char *oldpwfile = NULL;
static char *newpwfile = NULL;

void
usage( void )
{
	fprintf(stderr,
"Change password of an LDAP user\n\n"
"usage: %s [options] [user]\n"
"  user: the autentication identity, commonly a DN\n"
"Password change options:\n"
"  -a secret  old password\n"
"  -A         prompt for old password\n"
"  -t file    read file for old password\n"
"  -s secret  new password\n"
"  -S         prompt for new password\n"
"  -T file    read file for new password\n"
	        , prog );
	tool_common_usage();
	exit( EXIT_FAILURE );
}


const char options[] = "a:As:St:T:"
	"Cd:D:e:h:H:InO:p:QR:U:vVw:WxX:y:Y:Z";

int
handle_private_option( int i )
{
	switch ( i ) {
#if 0
	case 'E': /* passwd controls */ {
		int		crit;
		char	*control, *cvalue;
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

		fprintf( stderr, "Invalid passwd control name: %s\n", control );
		usage();
		}
#endif

	case 'a':	/* old password (secret) */
		oldpw.bv_val = strdup( optarg );
		{
			char* p;
			for( p = optarg; *p != '\0'; p++ ) {
				*p = '\0';
			}
		}
		oldpw.bv_len = strlen( oldpw.bv_val );
		break;

	case 'A':	/* prompt for old password */
		want_oldpw++;
		break;

	case 's':	/* new password (secret) */
		newpw.bv_val = strdup (optarg);
		{
			char* p;
			for( p = optarg; *p != '\0'; p++ ) {
				*p = '\0';
			}
		}
		newpw.bv_len = strlen( newpw.bv_val );
		break;

	case 'S':	/* prompt for user password */
		want_newpw++;
		break;

	case 't':
		oldpwfile = optarg;
		break;

	case 'T':
		newpwfile = optarg;
		break;

	default:
		return 0;
	}
	return 1;
}


int
main( int argc, char *argv[] )
{
	int rc;
	char	*user = NULL;

	LDAP	       *ld = NULL;
	struct berval bv = {0, NULL};
	BerElement  *ber = NULL;

	int id, code = LDAP_OTHER;
	LDAPMessage *res;
	char *matcheddn = NULL, *text = NULL, **refs = NULL;
	char	*retoid = NULL;
	struct berval *retdata = NULL;

	prog = lutil_progname( "ldappasswd", argc, argv );

	/* LDAPv3 only */
	protocol = LDAP_VERSION3;

	tool_args( argc, argv );

	if( argc - optind > 1 ) {
		usage();
	} else if ( argc - optind == 1 ) {
		user = strdup( argv[optind] );
	} else {
		user = NULL;
	}

	if( oldpwfile ) {
		rc = lutil_get_filed_password( prog, &oldpw );
		if( rc ) return EXIT_FAILURE;
	}

	if( want_oldpw && oldpw.bv_val == NULL ) {
		/* prompt for old password */
		char *ckoldpw;
		oldpw.bv_val = strdup(getpassphrase("Old password: "));
		ckoldpw = getpassphrase("Re-enter old password: ");

		if( oldpw.bv_val == NULL || ckoldpw == NULL ||
			strcmp( oldpw.bv_val, ckoldpw ))
		{
			fprintf( stderr, "passwords do not match\n" );
			return EXIT_FAILURE;
		}

		oldpw.bv_len = strlen( oldpw.bv_val );
	}

	if( newpwfile ) {
		rc = lutil_get_filed_password( prog, &newpw );
		if( rc ) return EXIT_FAILURE;
	}

	if( want_newpw && newpw.bv_val == NULL ) {
		/* prompt for new password */
		char *cknewpw;
		newpw.bv_val = strdup(getpassphrase("New password: "));
		cknewpw = getpassphrase("Re-enter new password: ");

		if( newpw.bv_val == NULL || cknewpw == NULL ||
			strcmp( newpw.bv_val, cknewpw ))
		{
			fprintf( stderr, "passwords do not match\n" );
			return EXIT_FAILURE;
		}

		newpw.bv_len = strlen( newpw.bv_val );
	}

	if( want_bindpw && passwd.bv_val == NULL ) {
		/* handle bind password */
		if ( pw_file ) {
			rc = lutil_get_filed_password( pw_file, &passwd );
			if( rc ) return EXIT_FAILURE;
		} else {
			passwd.bv_val = getpassphrase( "Enter LDAP Password: " );
			passwd.bv_len = passwd.bv_val ? strlen( passwd.bv_val ) : 0;
		}
	}

	ld = tool_conn_setup( 0, 0 );

	tool_bind( ld );

	if ( authzid || manageDSAit || noop )
		tool_server_controls( ld, NULL, 0 );

	if( user != NULL || oldpw.bv_val != NULL || newpw.bv_val != NULL ) {
		/* build change password control */
		ber = ber_alloc_t( LBER_USE_DER );

		if( ber == NULL ) {
			perror( "ber_alloc_t" );
			ldap_unbind( ld );
			return EXIT_FAILURE;
		}

		ber_printf( ber, "{" /*}*/ );

		if( user != NULL ) {
			ber_printf( ber, "ts",
				LDAP_TAG_EXOP_MODIFY_PASSWD_ID, user );
			free(user);
		}

		if( oldpw.bv_val != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_OLD, &oldpw );
			free(oldpw.bv_val);
		}

		if( newpw.bv_val != NULL ) {
			ber_printf( ber, "tO",
				LDAP_TAG_EXOP_MODIFY_PASSWD_NEW, &newpw );
			free(newpw.bv_val);
		}

		ber_printf( ber, /*{*/ "N}" );

		rc = ber_flatten2( ber, &bv, 0 );

		if( rc < 0 ) {
			perror( "ber_flatten2" );
			ldap_unbind( ld );
			return EXIT_FAILURE;
		}
	}

	if ( not ) {
		rc = LDAP_SUCCESS;
		goto skip;
	}

	rc = ldap_extended_operation( ld,
		LDAP_EXOP_MODIFY_PASSWD, bv.bv_val ? &bv : NULL, 
		NULL, NULL, &id );

	ber_free( ber, 1 );

	if( rc != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_extended_operation" );
		ldap_unbind( ld );
		return EXIT_FAILURE;
	}

	rc = ldap_result( ld, LDAP_RES_ANY, LDAP_MSG_ALL, NULL, &res );
	if ( rc < 0 ) {
		ldap_perror( ld, "ldappasswd: ldap_result" );
		return rc;
	}

	rc = ldap_parse_result( ld, res,
		&code, &matcheddn, &text, &refs, NULL, 0 );

	if( rc != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_parse_result" );
		return rc;
	}

	rc = ldap_parse_extended_result( ld, res, &retoid, &retdata, 1 );

	if( rc != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_parse_result" );
		return rc;
	}

	if( retdata != NULL ) {
		ber_tag_t tag;
		char *s;
		ber = ber_init( retdata );

		if( ber == NULL ) {
			perror( "ber_init" );
			ldap_unbind( ld );
			return EXIT_FAILURE;
		}

		/* we should check the tag */
		tag = ber_scanf( ber, "{a}", &s);

		if( tag == LBER_ERROR ) {
			perror( "ber_scanf" );
		} else {
			printf("New password: %s\n", s);
			free( s );
		}

		ber_free( ber, 1 );
	}

	if( verbose || code != LDAP_SUCCESS || matcheddn || text || refs ) {
		printf( "Result: %s (%d)\n", ldap_err2string( code ), code );

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
	ber_memfree( retoid );
	ber_bvfree( retdata );

skip:
	/* disconnect from server */
	ldap_unbind (ld);

	return code == LDAP_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
