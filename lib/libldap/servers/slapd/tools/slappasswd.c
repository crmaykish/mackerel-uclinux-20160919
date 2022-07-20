/* $OpenLDAP: pkg/ldap/servers/slapd/tools/slappasswd.c,v 1.12.2.3 2003/06/21 17:20:34 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <ldap.h>
#include <lutil.h>

#include "ldap_defaults.h"

static int	verbose = 0;

static void
usage(const char *s)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -h hash\tpassword scheme\n"
		"  -s secret\tnew password\n"
		"  -c format\tcrypt(3) salt format\n"
		"  -u\t\tgenerate RFC2307 values (default)\n"
		"  -v\t\tincrease verbosity\n"
		"  -T file\tread file for new password\n"
		, s );

	exit( EXIT_FAILURE );
}

int
main( int argc, char *argv[] )
{
	char	*scheme = "{SSHA}";
	char	*newpw = NULL;
	char	*pwfile = NULL;

	int		i;
	struct berval passwd;
	struct berval *hash = NULL;

	while( (i = getopt( argc, argv,
		"c:d:h:s:T:vu" )) != EOF )
	{
		switch (i) {
		case 'c':	/* crypt salt format */
			scheme = "{CRYPT}";
			lutil_salt_format( optarg );
			break;

		case 'h':	/* scheme */
			scheme = strdup( optarg );
			break;

		case 's':	/* new password (secret) */
			{
				char* p;
				newpw = strdup( optarg );

				for( p = optarg; *p != '\0'; p++ ) {
					*p = '\0';
				}
			} break;

		case 'T':	/* password file */
			pwfile = optarg;
			break;

		case 'u':	/* RFC2307 userPassword */
			break;

		case 'v':	/* verbose */
			verbose++;
			break;

		default:
			usage (argv[0]);
		}
	}

	if( argc - optind != 0 ) {
		usage( argv[0] );
	} 

	if( pwfile != NULL ) {
		if( lutil_get_filed_password( pwfile, &passwd )) {
			return EXIT_FAILURE;
		}
	} else {
		if( newpw == NULL ) {
			/* prompt for new password */
			char *cknewpw;
			newpw = strdup(getpassphrase("New password: "));
			cknewpw = getpassphrase("Re-enter new password: ");
	
			if( strcmp( newpw, cknewpw )) {
				fprintf( stderr, "Password values do not match\n" );
				return EXIT_FAILURE;
			}
		}

		passwd.bv_val = newpw;
		passwd.bv_len = strlen(passwd.bv_val);
	}

	hash = lutil_passwd_hash( &passwd, scheme );

	if( hash == NULL || hash->bv_val == NULL ) {
		fprintf( stderr, "Password generation failed.\n");
		return EXIT_FAILURE;
	}

	if( lutil_passwd( hash, &passwd, NULL ) ) {
		fprintf( stderr, "Password verification failed.\n");
		return EXIT_FAILURE;
	}

	printf( "%s\n" , hash->bv_val );
	return EXIT_SUCCESS;
}
