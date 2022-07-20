/* $OpenLDAP: pkg/ldap/servers/slapd/main.c,v 1.132.2.13 2003/03/27 03:04:06 hyc Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/wait.h>
#include <ac/errno.h>

#include "ldap_pvt.h"

#include "slap.h"
#include "lutil.h"
#include "ldif.h"

#ifdef LDAP_SLAPI
#include "slapi.h"
#endif

#ifdef LDAP_SIGCHLD
static RETSIGTYPE wait4child( int sig );
#endif

#ifdef HAVE_NT_SERVICE_MANAGER
#define MAIN_RETURN(x) return
static struct sockaddr_in	bind_addr;

#define SERVICE_EXIT( e, n )	do { \
	if ( is_NT_Service ) { \
		lutil_ServiceStatus.dwWin32ExitCode				= (e); \
		lutil_ServiceStatus.dwServiceSpecificExitCode	= (n); \
	} \
} while ( 0 )

#else
#define SERVICE_EXIT( e, n )
#define MAIN_RETURN(x) return(x)
#endif

/*
 * when more than one slapd is running on one machine, each one might have
 * it's own LOCAL for syslogging and must have its own pid/args files
 */

#ifndef HAVE_MKVERSION
const char Versionstr[] =
	OPENLDAP_PACKAGE " " OPENLDAP_VERSION " Standalone LDAP Server (slapd)";
#endif

#ifdef LOG_LOCAL4

#define DEFAULT_SYSLOG_USER  LOG_LOCAL4

typedef struct _str2intDispatch {
	char	*stringVal;
	int	 abbr;
	int	 intVal;
} STRDISP, *STRDISP_P;


/* table to compute syslog-options to integer */
static STRDISP	syslog_types[] = {
	{ "LOCAL0", sizeof("LOCAL0"), LOG_LOCAL0 },
	{ "LOCAL1", sizeof("LOCAL1"), LOG_LOCAL1 },
	{ "LOCAL2", sizeof("LOCAL2"), LOG_LOCAL2 },
	{ "LOCAL3", sizeof("LOCAL3"), LOG_LOCAL3 },
	{ "LOCAL4", sizeof("LOCAL4"), LOG_LOCAL4 },
	{ "LOCAL5", sizeof("LOCAL5"), LOG_LOCAL5 },
	{ "LOCAL6", sizeof("LOCAL6"), LOG_LOCAL6 },
	{ "LOCAL7", sizeof("LOCAL7"), LOG_LOCAL7 },
	{ NULL, 0, 0 }
};

static int   cnvt_str2int( char *, STRDISP_P, int );

#endif	/* LOG_LOCAL4 */

static int check_config = 0;

static void
usage( char *name )
{
	fprintf( stderr,
		"usage: %s options\n", name );
	fprintf( stderr,
		"\t-4\t\tIPv4 only\n"
		"\t-6\t\tIPv6 only\n"
		"\t-d level\tDebug level" "\n"
		"\t-f filename\tConfiguration file\n"
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
		"\t-g group\tGroup (id or name) to run as\n"
#endif
		"\t-h URLs\t\tList of URLs to serve\n"
#ifdef LOG_LOCAL4
		"\t-l facility\tSyslog facility (default: LOCAL4)\n"
#endif
		"\t-n serverName\tService name\n"
#ifdef HAVE_CHROOT
		"\t-r directory\tSandbox directory to chroot to\n"
#endif
		"\t-s level\tSyslog level\n"
		"\t-t\t\tCheck configuration file and exit\n"
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
		"\t-u user\t\tUser (id or name) to run as\n"
#endif
    );
}

#ifdef HAVE_NT_SERVICE_MANAGER
void WINAPI ServiceMain( DWORD argc, LPTSTR *argv )
#else
int main( int argc, char **argv )
#endif
{
	int		i, no_detach = 0;
	int		rc = 1;
	char *urls = NULL;
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
	char *username = NULL;
	char *groupname = NULL;
#endif
#if defined(HAVE_CHROOT)
	char *sandbox = NULL;
#endif
#ifdef LOG_LOCAL4
    int	    syslogUser = DEFAULT_SYSLOG_USER;
#endif
	
	int g_argc = argc;
	char **g_argv = argv;

#ifdef HAVE_NT_SERVICE_MANAGER
	char		*configfile = ".\\slapd.conf";
#else
	char		*configfile = SLAPD_DEFAULT_CONFIGFILE;
#endif
	char	    *serverName = NULL;
	int	    serverMode = SLAP_SERVER_MODE;

#ifdef CSRIMALLOC
	FILE *leakfile;
	if( ( leakfile = fopen( "slapd.leak", "w" )) == NULL ) {
		leakfile = stderr;
	}
#endif

#ifdef HAVE_NT_SERVICE_MANAGER
	{
		int *i;
		char *newConfigFile;
		char *newUrls;
		char *regService = NULL;

		if ( is_NT_Service ) {
			serverName = argv[0];
			lutil_CommenceStartupProcessing( serverName, slap_sig_shutdown );
			if ( strcmp(serverName, SERVICE_NAME) )
			    regService = serverName;
		}

		i = (int*)lutil_getRegParam( regService, "DebugLevel" );
		if ( i != NULL ) 
		{
			slap_debug = *i;
#ifdef NEW_LOGGING
			lutil_log_initialize( argc, argv );
			LDAP_LOG( SLAPD, INFO, 
				"main: new debug level from registry is: %d\n", 
				slap_debug, 0, 0 );
#else
			Debug( LDAP_DEBUG_ANY, "new debug level from registry is: %d\n", slap_debug, 0, 0 );
#endif
		}

		newUrls = (char *) lutil_getRegParam(regService, "Urls");
		if (newUrls)
		{
		    if (urls)
			ch_free(urls);

		    urls = ch_strdup(newUrls);
#ifdef NEW_LOGGING
		    LDAP_LOG( SLAPD, INFO, 
				"main: new urls from registry: %s\n", urls, 0, 0 );
#else
		    Debug(LDAP_DEBUG_ANY, "new urls from registry: %s\n",
			  urls, 0, 0);
#endif

		}

		newConfigFile = (char*)lutil_getRegParam( regService, "ConfigFile" );
		if ( newConfigFile != NULL ) 
		{
			configfile = newConfigFile;
#ifdef NEW_LOGGING
			LDAP_LOG( SLAPD, INFO, 
				"main: new config file from registry is: %s\n", configfile, 0, 0 );
#else
			Debug ( LDAP_DEBUG_ANY, "new config file from registry is: %s\n", configfile, 0, 0 );
#endif

		}
	}
#endif

	while ( (i = getopt( argc, argv,
			     "d:f:h:s:n:t"
#if LDAP_PF_INET6
				"46"
#endif
#ifdef HAVE_CHROOT
				"r:"
#endif
#ifdef LOG_LOCAL4
			     "l:"
#endif
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
			     "u:g:"
#endif
			     )) != EOF ) {
		switch ( i ) {
#ifdef LDAP_PF_INET6
		case '4':
			slap_inet4or6 = AF_INET;
			break;
		case '6':
			slap_inet4or6 = AF_INET6;
			break;
#endif

		case 'h':	/* listen URLs */
			if ( urls != NULL ) free( urls );
			urls = ch_strdup( optarg );
	    break;

		case 'd':	/* set debug level and 'do not detach' flag */
			no_detach = 1;
#ifdef LDAP_DEBUG
			slap_debug |= atoi( optarg );
#else
			if ( atoi( optarg ) != 0 )
				fputs( "must compile with LDAP_DEBUG for debugging\n",
				       stderr );
#endif
			break;

		case 'f':	/* read config file */
			configfile = ch_strdup( optarg );
			break;

		case 's':	/* set syslog level */
			ldap_syslog = atoi( optarg );
			break;

#ifdef LOG_LOCAL4
		case 'l':	/* set syslog local user */
			syslogUser = cnvt_str2int( optarg,
				syslog_types, DEFAULT_SYSLOG_USER );
			break;
#endif

#ifdef HAVE_CHROOT
		case 'r':
			if( sandbox ) free(sandbox);
			sandbox = ch_strdup( optarg );
			break;
#endif

#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
		case 'u':	/* user name */
			if( username ) free(username);
			username = ch_strdup( optarg );
			break;

		case 'g':	/* group name */
			if( groupname ) free(groupname);
			groupname = ch_strdup( optarg );
			break;
#endif /* SETUID && GETUID */

		case 'n':  /* NT service name */
			if( serverName != NULL ) free( serverName );
			serverName = ch_strdup( optarg );
			break;

		case 't':
			check_config++;
			break;

		default:
			usage( argv[0] );
			rc = 1;
			SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 15 );
			goto stop;
		}
	}

#ifdef NEW_LOGGING
	lutil_log_initialize( argc, argv );
#else
	lutil_set_debug_level( "slapd", slap_debug );
	ber_set_option(NULL, LBER_OPT_DEBUG_LEVEL, &slap_debug);
	ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, &slap_debug);
	ldif_debug = slap_debug;
#endif

#ifdef NEW_LOGGING
	LDAP_LOG( SLAPD, INFO, "%s", Versionstr, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "%s", Versionstr, 0, 0 );
#endif

	if( serverName == NULL ) {
		if ( (serverName = strrchr( argv[0], *LDAP_DIRSEP )) == NULL ) {
			serverName = argv[0];
		} else {
			serverName = serverName + 1;
		}
	}

#ifdef LOG_LOCAL4
	openlog( serverName, OPENLOG_OPTIONS, syslogUser );
#elif LOG_DEBUG
	openlog( serverName, OPENLOG_OPTIONS );
#endif

	if( !check_config && slapd_daemon_init( urls ) != 0 ) {
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 16 );
		goto stop;
	}

#if defined(HAVE_CHROOT)
	if ( sandbox ) {
		if ( chdir( sandbox ) ) {
			perror("chdir");
			rc = 1;
			goto stop;
		}
		if ( chroot( sandbox ) ) {
			perror("chroot");
			rc = 1;
			goto stop;
		}
	}
#endif

#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
	if ( username != NULL || groupname != NULL ) {
		slap_init_user( username, groupname );
	}
#endif

	extops_init();
 	slap_op_init();

#ifdef SLAPD_MODULES
	if ( module_init() != 0 ) {
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 17 );
		goto destroy;
	}
#endif

	if ( slap_init( serverMode, serverName ) != 0 ) {
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 18 );
		goto destroy;
	}

	if ( slap_schema_init( ) != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT, "main: schema initialization error\n", 0, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "schema initialization error\n",
		    0, 0, 0 );
#endif

		goto destroy;
	}

#ifdef HAVE_TLS
	/* Library defaults to full certificate checking. This is correct when
	 * a client is verifying a server because all servers should have a
	 * valid cert. But few clients have valid certs, so we want our default
	 * to be no checking. The config file can override this as usual.
	 */
	rc = 0;
	(void) ldap_pvt_tls_set_option( NULL, LDAP_OPT_X_TLS_REQUIRE_CERT, &rc );
#endif

#ifdef LDAP_SLAPI
	if ( slapi_init() != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( OPERATION, CRIT, "main: slapi initialization error\n", 0, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "slapi initialization error\n",
		    0, 0, 0 );
#endif

		goto destroy;
	}
#endif /* LDAP_SLAPI */

	if ( read_config( configfile, 0 ) != 0 ) {
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 19 );

		if ( check_config ) {
			fprintf( stderr, "config check failed\n" );
		}

		goto destroy;
	}

	if ( check_config ) {
		rc = 0;
		fprintf( stderr, "config check succeeded\n" );
		goto destroy;
	}

	if ( glue_sub_init( ) != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( SLAPD, CRIT, "main: subordinate config error\n", 0, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "subordinate config error\n",
		    0, 0, 0 );
#endif
		goto destroy;
	}

	if ( slap_schema_check( ) != 0 ) {
#ifdef NEW_LOGGING
		LDAP_LOG( SLAPD, CRIT, "main: schema prep error\n", 0, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "schema prep error\n",
		    0, 0, 0 );
#endif

		goto destroy;
	}

#ifdef HAVE_TLS
	rc = ldap_pvt_tls_init();
	if( rc != 0) {
#ifdef NEW_LOGGING
		LDAP_LOG( SLAPD, CRIT, "main: tls init failed: %d\n", rc, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "main: TLS init failed: %d\n",
		    0, 0, 0 );
#endif
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 20 );
		goto destroy;
	}

	rc = ldap_pvt_tls_init_def_ctx();
	if( rc != 0) {
#ifdef NEW_LOGGING
		LDAP_LOG( SLAPD, CRIT, "main: tls init def ctx failed: %d\n", rc, 0, 0 );
#else
		Debug( LDAP_DEBUG_ANY,
		    "main: TLS init def ctx failed: %d\n",
		    rc, 0, 0 );
#endif
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 20 );
		goto destroy;
	}
#endif

	(void) SIGNAL( LDAP_SIGUSR1, slap_sig_wake );
	(void) SIGNAL( LDAP_SIGUSR2, slap_sig_shutdown );

#ifdef SIGPIPE
	(void) SIGNAL( SIGPIPE, SIG_IGN );
#endif
#ifdef SIGHUP
	(void) SIGNAL( SIGHUP, slap_sig_shutdown );
#endif
	(void) SIGNAL( SIGINT, slap_sig_shutdown );
	(void) SIGNAL( SIGTERM, slap_sig_shutdown );
#ifdef LDAP_SIGCHLD
	(void) SIGNAL( LDAP_SIGCHLD, wait4child );
#endif
#ifdef SIGBREAK
	/* SIGBREAK is generated when Ctrl-Break is pressed. */
	(void) SIGNAL( SIGBREAK, slap_sig_shutdown );
#endif

#ifndef HAVE_WINSOCK
	lutil_detach( no_detach, 0 );
#endif /* HAVE_WINSOCK */

#ifdef CSRIMALLOC
	mal_leaktrace(1);
#endif

	if ( slap_startup( NULL )  != 0 ) {
		rc = 1;
		SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 21 );
		goto shutdown;
	}

#ifdef NEW_LOGGING
	LDAP_LOG( SLAPD, INFO, "main: slapd starting.\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY, "slapd starting\n", 0, 0, 0 );
#endif


	if ( slapd_pid_file != NULL ) {
		FILE *fp = fopen( slapd_pid_file, "w" );

		if( fp != NULL ) {
			fprintf( fp, "%d\n", (int) getpid() );
			fclose( fp );

		} else {
			free(slapd_pid_file);
			slapd_pid_file = NULL;
		}
	}

	if ( slapd_args_file != NULL ) {
		FILE *fp = fopen( slapd_args_file, "w" );

		if( fp != NULL ) {
			for ( i = 0; i < g_argc; i++ ) {
				fprintf( fp, "%s ", g_argv[i] );
			}
			fprintf( fp, "\n" );
			fclose( fp );
		} else {
			free(slapd_args_file);
			slapd_args_file = NULL;
		}
	}

#ifdef HAVE_NT_EVENT_LOG
	if (is_NT_Service)
	lutil_LogStartedEvent( serverName, slap_debug, configfile, urls );
#endif

	rc = slapd_daemon();

#ifdef HAVE_NT_SERVICE_MANAGER
	/* Throw away the event that we used during the startup process. */
	if ( is_NT_Service )
		ldap_pvt_thread_cond_destroy( &started_event );
#endif

shutdown:
	/* remember an error during shutdown */
	rc |= slap_shutdown( NULL );

destroy:
	/* remember an error during destroy */
	rc |= slap_destroy();

#ifdef SLAPD_MODULES
	module_kill();
#endif

	slap_op_destroy();

	extops_kill();

stop:
#ifdef HAVE_NT_EVENT_LOG
	if (is_NT_Service)
	lutil_LogStoppedEvent( serverName );
#endif

#ifdef NEW_LOGGING
	LDAP_LOG( SLAPD, CRIT, "main: slapd stopped.\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_ANY, "slapd stopped.\n", 0, 0, 0 );
#endif


#ifdef HAVE_NT_SERVICE_MANAGER
	lutil_ReportShutdownComplete();
#endif

#ifdef LOG_DEBUG
    closelog();
#endif
	slapd_daemon_destroy();

	schema_destroy();

#ifdef HAVE_TLS
	ldap_pvt_tls_destroy();
#endif

	if ( slapd_pid_file != NULL ) {
		unlink( slapd_pid_file );
	}
	if ( slapd_args_file != NULL ) {
		unlink( slapd_args_file );
	}

	config_destroy();

#ifdef CSRIMALLOC
	mal_dumpleaktrace( leakfile );
#endif

	MAIN_RETURN(rc);
}


#ifdef LDAP_SIGCHLD

/*
 *  Catch and discard terminated child processes, to avoid zombies.
 */

static RETSIGTYPE
wait4child( int sig )
{
    int save_errno = errno;

#ifdef WNOHANG
    errno = 0;
#ifdef HAVE_WAITPID
    while ( waitpid( (pid_t)-1, NULL, WNOHANG ) > 0 || errno == EINTR )
	;	/* NULL */
#else
    while ( wait3( NULL, WNOHANG, NULL ) > 0 || errno == EINTR )
	;	/* NULL */
#endif
#else
    (void) wait( NULL );
#endif
    (void) SIGNAL_REINSTALL( sig, wait4child );
    errno = save_errno;
}

#endif /* LDAP_SIGCHLD */


#ifdef LOG_LOCAL4

/*
 *  Convert a string to an integer by means of a dispatcher table
 *  if the string is not in the table return the default
 */

static int
cnvt_str2int( char *stringVal, STRDISP_P dispatcher, int defaultVal )
{
    int	       retVal = defaultVal;
    STRDISP_P  disp;

    for (disp = dispatcher; disp->stringVal; disp++) {

	if (!strncasecmp (stringVal, disp->stringVal, disp->abbr)) {

	    retVal = disp->intVal;
	    break;

	}
    }

    return (retVal);
}

#endif	/* LOG_LOCAL4 */
