/* $OpenLDAP: pkg/ldap/tests/progs/slapd-modrdn.c,v 1.1.2.3 2003/04/30 03:13:57 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/param.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/wait.h>

#include <ldap.h>

#define LOOPS	100

static void
do_modrdn( char *uri, char *host, int port, char *manager, char *passwd, char *entry, int maxloop );

static void
usage( char *name )
{
	fprintf( stderr, "usage: %s [-h <host>] -p port -D <managerDN> -w <passwd> -e <entry> [-l <loops>]\n",
			name );
	exit( EXIT_FAILURE );
}

int
main( int argc, char **argv )
{
	int		i;
	char		*uri = NULL;
	char        *host = "localhost";
	int			port = -1;
	char		*manager = NULL;
	char		*passwd = NULL;
	char		*entry = NULL;
	int			loops = LOOPS;

	while ( (i = getopt( argc, argv, "H:h:p:D:w:e:l:" )) != EOF ) {
		switch( i ) {
			case 'H':		/* the server uri */
				uri = strdup( optarg );
			break;
			case 'h':		/* the servers host */
				host = strdup( optarg );
			break;

			case 'p':		/* the servers port */
				port = atoi( optarg );
				break;
			case 'D':		/* the servers manager */
				manager = strdup( optarg );
			break;

			case 'w':		/* the server managers password */
				passwd = strdup( optarg );
			break;
			case 'e':		/* entry to rename */
				entry = strdup( optarg );
				break;

			case 'l':		/* the number of loops */
				loops = atoi( optarg );
				break;

			default:
				usage( argv[0] );
				break;
		}
	}

	if (( entry == NULL ) || ( port == -1 && uri == NULL ))
		usage( argv[0] );

	if ( *entry == '\0' ) {

		fprintf( stderr, "%s: invalid EMPTY entry DN.\n",
				argv[0] );
		exit( EXIT_FAILURE );

	}

	do_modrdn( uri, host, port, manager, passwd, entry, loops );
	exit( EXIT_SUCCESS );
}


static void
do_modrdn( char *uri, char *host, int port, char *manager,
	char *passwd, char *entry, int maxloop )
{
	LDAP	*ld = NULL;
	int  	i;
	pid_t	pid;
	char *DNs[2];
	char *rdns[2];

	pid = getpid();
	DNs[0] = entry;
	DNs[1] = strdup( entry );

	/* reverse the RDN, make new DN */
	{
		char *p1, *p2;

		p1 = strchr( entry, '=' ) + 1;
		p2 = strchr( p1, ',' );

		*p2 = '\0';
		rdns[1] = strdup( entry );
		*p2-- = ',';

		for (i = p1 - entry;p2 >= p1;)
			DNs[1][i++] = *p2--;
		
		DNs[1][i] = '\0';
		rdns[0] = strdup( DNs[1] );
		DNs[1][i] = ',';
	}
		
	if ( uri ) {
		ldap_initialize( &ld, uri );
	} else {
		ld = ldap_init( host, port );
	}
	if ( ld == NULL ) {
		perror( "ldap_init" );
		exit( EXIT_FAILURE );
	}

	{
		int version = LDAP_VERSION3;
		(void) ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION,
			&version ); 
	}

	if ( ldap_bind_s( ld, manager, passwd, LDAP_AUTH_SIMPLE ) != LDAP_SUCCESS ) {
		ldap_perror( ld, "ldap_bind" );
		 exit( EXIT_FAILURE );
	}


	fprintf( stderr, "PID=%ld - Modrdn(%d): entry=\"%s\".\n",
		 (long) pid, maxloop, entry );

	for ( i = 0; i < maxloop; i++ ) {
		int         rc;

		if (( rc = ldap_modrdn2_s( ld, DNs[0], rdns[0], 0 ))
			!= LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_modrdn" );
			if ( rc != LDAP_NO_SUCH_OBJECT ) break;
			continue;
		}
		if (( rc = ldap_modrdn2_s( ld, DNs[1], rdns[1], 1 ))
			!= LDAP_SUCCESS ) {
			ldap_perror( ld, "ldap_modrdn" );
			if ( rc != LDAP_NO_SUCH_OBJECT ) break;
			continue;
		}
	}

	fprintf( stderr, " PID=%ld - Modrdn done.\n", (long) pid );

	ldap_unbind( ld );
}


