/* $OpenLDAP: pkg/ldap/clients/tools/common.c,v 1.5.2.9 2003/05/22 22:22:17 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* common.c - common routines for the ldap client tools */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/signal.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/errno.h>

#include <ldap.h>

#include "lutil_ldap.h"

#include "common.h"


int   authmethod = -1;
char *binddn = NULL;
int   contoper = 0;
int   debug = 0;
char *infile = NULL;
char *ldapuri = NULL;
char *ldaphost = NULL;
int   ldapport = 0;
#ifdef HAVE_CYRUS_SASL
unsigned sasl_flags = LDAP_SASL_AUTOMATIC;
char	*sasl_realm = NULL;
char	*sasl_authc_id = NULL;
char	*sasl_authz_id = NULL;
char	*sasl_mech = NULL;
char	*sasl_secprops = NULL;
#endif
int   use_tls = 0;

char *authzid = NULL;
int   manageDSAit = 0;
int   noop = 0;

int   not = 0;
int   want_bindpw = 0;
struct berval passwd = { 0, NULL };
char *pw_file = NULL;
int   referrals = 0;
int   protocol = -1;
int   verbose = 0;
int   version = 0;

/* Set in main() */
char *prog = NULL;

void
tool_common_usage( void )
{
	static const char *const descriptions[] = {
"  -c         continuous operation mode (do not stop on errors)\n",
"  -C         chase referrals\n",
"  -d level   set LDAP debugging level to `level'\n",
"  -D binddn  bind DN\n",
"  -e [!]<ctrl>[=<ctrlparam>] general controls (! indicates criticality)\n"
"             [!]authzid=<authzid> (\"dn:<dn>\" or \"u:<user>\")\n"
"             [!]manageDSAit       (alternate form, see -M)\n"
"             [!]noop\n",
"  -f file    read operations from `file'\n",
"  -h host    LDAP server\n",
"  -H URI     LDAP Uniform Resource Indentifier(s)\n",
"  -I         use SASL Interactive mode\n",
"  -k         use Kerberos authentication\n",
"  -K         like -k, but do only step 1 of the Kerberos bind\n",
"  -M         enable Manage DSA IT control (-MM to make critical)\n",
"  -n         show what would be done but don't actually do it\n",
"  -O props   SASL security properties\n",
"  -p port    port on LDAP server\n",
"  -P version procotol version (default: 3)\n",
"  -Q         use SASL Quiet mode\n",
"  -R realm   SASL realm\n",
"  -U authcid SASL authentication identity\n",
"  -v         run in verbose mode (diagnostics to standard output)\n",
"  -V         print version info (-VV only)\n",
"  -w passwd  bind password (for simple authentication)\n",
"  -W         prompt for bind password\n",
"  -x         Simple authentication\n",
"  -X authzid SASL authorization identity (\"dn:<dn>\" or \"u:<user>\")\n",
"  -y file    Read password from file\n",
"  -Y mech    SASL mechanism\n",
"  -Z         Start TLS request (-ZZ to require successful response)\n",
NULL
	};
	const char *const *cpp;

	fputs( "Common options:\n", stderr );
	for( cpp = descriptions; *cpp != NULL; cpp++ ) {
		if( strchr( options, (*cpp)[3] ) ) {
			fputs( *cpp, stderr );
		}
	}
}


void
tool_args( int argc, char **argv )
{
	int i;
    while (( i = getopt( argc, argv, options )) != EOF )
	{
		int crit;
		char *control, *cvalue;
		switch( i ) {
		case 'c':	/* continuous operation mode */
			contoper++;
			break;
		case 'C':
			referrals++;
			break;
		case 'd':
			debug |= atoi( optarg );
			break;
		case 'D':	/* bind DN */
			if( binddn != NULL ) {
				fprintf( stderr, "%s: -D previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			binddn = ber_strdup( optarg );
			break;
		case 'e': /* general controls */
			/* should be extended to support comma separated list of
			 *	[!]key[=value] parameters, e.g.  -e !foo,bar=567
			 */

			crit = 0;
			cvalue = NULL;
			if( optarg[0] == '!' ) {
				crit = 1;
				optarg++;
			}

			control = ber_strdup( optarg );
			if ( (cvalue = strchr( control, '=' )) != NULL ) {
				*cvalue++ = '\0';
			}

			if ( strcasecmp( control, "authzid" ) == 0 ) {
				if( authzid != NULL ) {
					fprintf( stderr, "authzid control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue == NULL ) {
					fprintf( stderr, "authzid: control value expected\n" );
					usage();
				}
				if( !crit ) {
					fprintf( stderr, "authzid: must be marked critical\n" );
					usage();
				}

				assert( authzid == NULL );
				authzid = cvalue;

			} else if ( strcasecmp( control, "manageDSAit" ) == 0 ) {
				if( manageDSAit ) {
					fprintf( stderr,
					         "manageDSAit control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr,
					         "manageDSAit: no control value expected\n" );
					usage();
				}

				manageDSAit = 1 + crit;

			} else if ( strcasecmp( control, "noop" ) == 0 ) {
				if( noop ) {
					fprintf( stderr, "noop control previously specified\n");
					exit( EXIT_FAILURE );
				}
				if( cvalue != NULL ) {
					fprintf( stderr, "noop: no control value expected\n" );
					usage();
				}

				noop = 1 + crit;

			} else {
				fprintf( stderr, "Invalid general control name: %s\n",
				         control );
				usage();
			}
			break;
		case 'f':	/* read from file */
			if( infile != NULL ) {
				fprintf( stderr, "%s: -f previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			infile = ber_strdup( optarg );
			break;
		case 'h':	/* ldap host */
			if( ldaphost != NULL ) {
				fprintf( stderr, "%s: -h previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			ldaphost = ber_strdup( optarg );
			break;
		case 'H':	/* ldap URI */
			if( ldapuri != NULL ) {
				fprintf( stderr, "%s: -H previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			ldapuri = ber_strdup( optarg );
			break;
		case 'I':
#ifdef HAVE_CYRUS_SASL
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
						 "authentication choice\n",
						 prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_flags = LDAP_SASL_INTERACTIVE;
			break;
#else
			fprintf( stderr, "%s: was not compiled with SASL support\n",
					 prog );
			exit( EXIT_FAILURE );
#endif
		case 'k':	/* kerberos bind */
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
			if( authmethod != -1 ) {
				fprintf( stderr, "%s: -k incompatible with previous "
						 "authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_KRBV4;
#else
			fprintf( stderr, "%s: not compiled with Kerberos support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'K':	/* kerberos bind, part one only */
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
			if( authmethod != -1 ) {
				fprintf( stderr, "%s: incompatible with previous "
						 "authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_KRBV41;
#else
			fprintf( stderr, "%s: not compiled with Kerberos support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'M':
			/* enable Manage DSA IT */
			manageDSAit++;
			break;
		case 'n':	/* print operations, don't actually do them */
			not++;
			break;
		case 'O':
#ifdef HAVE_CYRUS_SASL
			if( sasl_secprops != NULL ) {
				fprintf( stderr, "%s: -O previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
						 "authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_secprops = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
					 prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'p':
			if( ldapport ) {
				fprintf( stderr, "%s: -p previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			ldapport = atoi( optarg );
			break;
		case 'P':
			switch( atoi(optarg) ) {
			case 2:
				if( protocol == LDAP_VERSION3 ) {
					fprintf( stderr, "%s: -P 2 incompatible with version %d\n",
							 prog, protocol );
					exit( EXIT_FAILURE );
				}
				protocol = LDAP_VERSION2;
				break;
			case 3:
				if( protocol == LDAP_VERSION2 ) {
					fprintf( stderr, "%s: -P 2 incompatible with version %d\n",
							 prog, protocol );
					exit( EXIT_FAILURE );
				}
				protocol = LDAP_VERSION3;
				break;
			default:
				fprintf( stderr, "%s: protocol version should be 2 or 3\n",
						 prog );
				usage();
			}
			break;
		case 'Q':
#ifdef HAVE_CYRUS_SASL
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
						 "authentication choice\n",
						 prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_flags = LDAP_SASL_QUIET;
			break;
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
					 prog );
			exit( EXIT_FAILURE );
#endif
		case 'R':
#ifdef HAVE_CYRUS_SASL
			if( sasl_realm != NULL ) {
				fprintf( stderr, "%s: -R previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
						 "authentication choice\n",
						 prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_realm = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
					 prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'U':
#ifdef HAVE_CYRUS_SASL
			if( sasl_authc_id != NULL ) {
				fprintf( stderr, "%s: -U previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible previous "
						 "authentication choice\n",
						 prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_authc_id = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
					 prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'v':	/* verbose mode */
			verbose++;
			break;
		case 'V':	/* version */
			version++;
			break;
		case 'w':	/* password */
			passwd.bv_val = ber_strdup( optarg );
			{
				char* p;

				for( p = optarg; *p != '\0'; p++ ) {
					*p = '\0';
				}
			}
			passwd.bv_len = strlen( passwd.bv_val );
			break;
		case 'W':
			want_bindpw++;
			break;
		case 'y':
			pw_file = optarg;
			break;
		case 'Y':
#ifdef HAVE_CYRUS_SASL
			if( sasl_mech != NULL ) {
				fprintf( stderr, "%s: -Y previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: incompatible with authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_mech = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n",
					 prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'x':
			if( authmethod != -1 && authmethod != LDAP_AUTH_SIMPLE ) {
				fprintf( stderr, "%s: incompatible with previous "
						 "authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SIMPLE;
			break;
		case 'X':
#ifdef HAVE_CYRUS_SASL
			if( sasl_authz_id != NULL ) {
				fprintf( stderr, "%s: -X previously specified\n", prog );
				exit( EXIT_FAILURE );
			}
			if( authmethod != -1 && authmethod != LDAP_AUTH_SASL ) {
				fprintf( stderr, "%s: -X incompatible with "
						 "authentication choice\n", prog );
				exit( EXIT_FAILURE );
			}
			authmethod = LDAP_AUTH_SASL;
			sasl_authz_id = ber_strdup( optarg );
#else
			fprintf( stderr, "%s: not compiled with SASL support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		case 'Z':
#ifdef HAVE_TLS
			use_tls++;
#else
			fprintf( stderr, "%s: not compiled with TLS support\n", prog );
			exit( EXIT_FAILURE );
#endif
			break;
		default:
			if( handle_private_option( i ) )
				break;
			fprintf( stderr, "%s: unrecognized option -%c\n",
					 prog, optopt );
			usage();
		}
    }

	{
		/* prevent bad linking */
		LDAPAPIInfo api;
		api.ldapai_info_version = LDAP_API_INFO_VERSION;

		if ( ldap_get_option(NULL, LDAP_OPT_API_INFO, &api)
			!= LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "%s: ldap_get_option(API_INFO) failed\n", prog );
			exit( EXIT_FAILURE );
		}

		if (api.ldapai_info_version != LDAP_API_INFO_VERSION) {
			fprintf( stderr, "LDAP APIInfo version mismatch: "
				"got %d, expected %d\n",
				api.ldapai_info_version, LDAP_API_INFO_VERSION );
			exit( EXIT_FAILURE );
    	}

		if( api.ldapai_api_version != LDAP_API_VERSION ) {
			fprintf( stderr, "LDAP API version mismatch: "
				"got %d, expected %d\n",
				api.ldapai_api_version, LDAP_API_VERSION );
			exit( EXIT_FAILURE );
		}

		if( strcmp(api.ldapai_vendor_name, LDAP_VENDOR_NAME ) != 0 ) {
			fprintf( stderr, "LDAP vendor name mismatch: "
				"got %s, expected %s\n",
				api.ldapai_vendor_name, LDAP_VENDOR_NAME );
			exit( EXIT_FAILURE );
		}

		if( api.ldapai_vendor_version != LDAP_VENDOR_VERSION ) {
			fprintf( stderr, "LDAP vendor version mismatch: "
				"got %d, expected %d\n",
				api.ldapai_vendor_version, LDAP_VENDOR_VERSION );
			exit( EXIT_FAILURE );
		}

		if (version) {
			fprintf( stderr, "%s: %s\t(LDAP library: %s %d)\n",
				prog, __Version,
				LDAP_VENDOR_NAME, LDAP_VENDOR_VERSION );
			if (version > 1) exit( EXIT_SUCCESS );
		}
	}

	if (protocol == -1)
		protocol = LDAP_VERSION3;

	if (authmethod == -1 && protocol > LDAP_VERSION2) {
#ifdef HAVE_CYRUS_SASL
		authmethod = LDAP_AUTH_SASL;
#else
		authmethod = LDAP_AUTH_SIMPLE;
#endif
	}

	if( ldapuri == NULL ) {
		if( ldapport && ( ldaphost == NULL )) {
			fprintf( stderr, "%s: -p without -h is invalid.\n", prog );
			exit( EXIT_FAILURE );
		}
	} else {
		if( ldaphost != NULL ) {
			fprintf( stderr, "%s: -H incompatible with -h\n", prog );
			exit( EXIT_FAILURE );
		}
		if( ldapport ) {
			fprintf( stderr, "%s: -H incompatible with -p\n", prog );
			exit( EXIT_FAILURE );
		}
	}
	if( protocol == LDAP_VERSION2 ) {
		if( authzid || manageDSAit || noop ) {
			fprintf( stderr, "%s: -e/-M incompatible with LDAPv2\n", prog );
			exit( EXIT_FAILURE );
		}
#ifdef HAVE_TLS
		if( use_tls ) {
			fprintf( stderr, "%s: -Z incompatible with LDAPv2\n", prog );
			exit( EXIT_FAILURE );
		}
#endif
#ifdef HAVE_CYRUS_SASL
		if( authmethod == LDAP_AUTH_SASL ) {
			fprintf( stderr, "%s: -[IOQRUXY] incompatible with LDAPv2\n",
			         prog );
			exit( EXIT_FAILURE );
		}
#endif
	} else {
#ifdef LDAP_API_FEATURE_X_OPENLDAP_V2_KBIND
		if ( authmethod == LDAP_AUTH_KRBV4 || authmethod == LDAP_AUTH_KRBV41 ) {
			fprintf( stderr, "%s: -k/-K incompatible with LDAPv%d\n",
			         prog, protocol );
			exit( EXIT_FAILURE );
		}
#endif
	}
}


LDAP *
tool_conn_setup( int not, void (*private_setup)( LDAP * ) )
{
	LDAP *ld = NULL;

	if ( debug ) {
		if( ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &debug )
		    != LBER_OPT_SUCCESS ) {
			fprintf( stderr, "Could not set LBER_OPT_DEBUG_LEVEL %d\n", debug );
		}
		if( ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &debug )
		    != LDAP_OPT_SUCCESS ) {
			fprintf( stderr, "Could not set LDAP_OPT_DEBUG_LEVEL %d\n", debug );
		}
	}

#ifdef SIGPIPE
	(void) SIGNAL( SIGPIPE, SIG_IGN );
#endif

	if ( !not ) {
		/* connect to server */
		if( ( ldaphost != NULL || ldapport ) && ( ldapuri == NULL ) ) {
			if ( verbose ) {
				fprintf( stderr, "ldap_init( %s, %d )\n",
				         ldaphost != NULL ? ldaphost : "<DEFAULT>",
				         ldapport );
			}

			ld = ldap_init( ldaphost, ldapport );
			if( ld == NULL ) {
				char buf[20 + sizeof(": ldap_init")];
				sprintf( buf, "%.20s: ldap_init", prog );
				perror( buf );
				exit( EXIT_FAILURE );
			}

		} else {
			int rc;
			if ( verbose ) {
				fprintf( stderr, "ldap_initialize( %s )\n",
				         ldapuri != NULL ? ldapuri : "<DEFAULT>" );
			}
			rc = ldap_initialize( &ld, ldapuri );
			if( rc != LDAP_SUCCESS ) {
				fprintf( stderr, "Could not create LDAP session handle (%d): %s\n",
				         rc, ldap_err2string(rc) );
				exit( EXIT_FAILURE );
			}
		}

		if( private_setup )
			private_setup( ld );

		/* referrals */
		if( ldap_set_option( ld, LDAP_OPT_REFERRALS,
			                 referrals ? LDAP_OPT_ON : LDAP_OPT_OFF )
			!= LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "Could not set LDAP_OPT_REFERRALS %s\n",
			         referrals ? "on" : "off" );
			exit( EXIT_FAILURE );
		}

		if( ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &protocol )
		    != LDAP_OPT_SUCCESS )
		{
			fprintf( stderr, "Could not set LDAP_OPT_PROTOCOL_VERSION %d\n",
			         protocol );
			exit( EXIT_FAILURE );
		}

		if ( use_tls &&
		     ( ldap_start_tls_s( ld, NULL, NULL ) != LDAP_SUCCESS )) {
			ldap_perror( ld, "ldap_start_tls" );
			if ( use_tls > 1 ) {
				exit( EXIT_FAILURE );
			}
		}
	}

	return ld;
}


void
tool_bind( LDAP *ld )
{
	if ( authmethod == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void *defaults;
		int rc;

		if( sasl_secprops != NULL ) {
			rc = ldap_set_option( ld, LDAP_OPT_X_SASL_SECPROPS,
				(void *) sasl_secprops );

			if( rc != LDAP_OPT_SUCCESS ) {
				fprintf( stderr,
					"Could not set LDAP_OPT_X_SASL_SECPROPS: %s\n",
					sasl_secprops );
				exit( EXIT_FAILURE );
			}
		}

		defaults = lutil_sasl_defaults( ld,
			sasl_mech,
			sasl_realm,
			sasl_authc_id,
			passwd.bv_val,
			sasl_authz_id );

		rc = ldap_sasl_interactive_bind_s( ld, binddn,
			sasl_mech, NULL, NULL,
			sasl_flags, lutil_sasl_interact, defaults );

		lutil_sasl_freedefs( defaults );
		if( rc != LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_sasl_interactive_bind_s" );
			exit( EXIT_FAILURE );
		}
#else
		fprintf( stderr, "%s: not compiled with SASL support\n",
			prog );
		exit( EXIT_FAILURE );
#endif
	} else {
		if ( ldap_bind_s( ld, binddn, passwd.bv_val, authmethod )
		     != LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_bind" );
			exit( EXIT_FAILURE );
		}
	}
}


/* Set server controls.  Add controls extra_c[0..count-1], if set. */
void
tool_server_controls( LDAP *ld, LDAPControl *extra_c, int count )
{
	int i = 0, j, crit = 0, err;
	LDAPControl c[3], **ctrls;

	ctrls = (LDAPControl**) malloc(sizeof(c) + (count+1)*sizeof(LDAPControl*));
	if ( ctrls == NULL ) {
		fprintf( stderr, "No memory\n" );
		exit( EXIT_FAILURE );
	}

	if ( authzid ) {
		c[i].ldctl_oid = LDAP_CONTROL_PROXY_AUTHZ;
		c[i].ldctl_value.bv_val = authzid;
		c[i].ldctl_value.bv_len = strlen( authzid );
		c[i].ldctl_iscritical = 1;
		ctrls[i] = &c[i];
		i++;
	}

	if ( manageDSAit ) {
		c[i].ldctl_oid = LDAP_CONTROL_MANAGEDSAIT;
		c[i].ldctl_value.bv_val = NULL;
		c[i].ldctl_value.bv_len = 0;
		c[i].ldctl_iscritical = manageDSAit > 1;
		ctrls[i] = &c[i];
		i++;
	}

	if ( noop ) {
		c[i].ldctl_oid = LDAP_CONTROL_NOOP;
		c[i].ldctl_value.bv_val = NULL;
		c[i].ldctl_value.bv_len = 0;
		c[i].ldctl_iscritical = noop > 1;
		ctrls[i] = &c[i];
		i++;
	}
	
	while ( count-- )
		ctrls[i++] = extra_c++;
	ctrls[i] = NULL;

	err = ldap_set_option( ld, LDAP_OPT_SERVER_CONTROLS, ctrls );

	if ( err != LDAP_OPT_SUCCESS ) {
		for ( j = 0; j < i; j++ )
			if ( ctrls[j]->ldctl_iscritical )
				crit = 1;
		fprintf( stderr, "Could not set %scontrols\n",
				 crit ? "critical " : "" );
	}

 	free( ctrls );
	if ( crit ) {
		exit( EXIT_FAILURE );
	}
}
