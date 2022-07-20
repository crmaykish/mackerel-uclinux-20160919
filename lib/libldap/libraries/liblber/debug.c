/* $OpenLDAP: pkg/ldap/libraries/liblber/debug.c,v 1.4.2.5 2003/03/03 17:10:04 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdarg.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/ctype.h>

#ifdef LDAP_SYSLOG
#include <ac/syslog.h>
#endif

#include "ldap_log.h"
#include "ldap_defaults.h"
#include "lber.h"
#include "ldap_pvt.h"

struct DEBUGLEVEL
{
	char *subsystem;
	int  level;
};

int ldap_loglevels[LDAP_SUBSYS_NUM];

static FILE *log_file = NULL;
static int global_level = 0;

#ifdef LDAP_SYSLOG
static int use_syslog = 0;

static int debug2syslog(int l) {
	switch (l) {
	case LDAP_LEVEL_EMERG:	return LOG_EMERG;
	case LDAP_LEVEL_ALERT:	return LOG_ALERT;
	case LDAP_LEVEL_CRIT:	return LOG_CRIT;
	case LDAP_LEVEL_ERR:	return LOG_ERR;
	case LDAP_LEVEL_WARNING:	return LOG_WARNING;
	case LDAP_LEVEL_NOTICE:	return LOG_NOTICE;
	case LDAP_LEVEL_INFO:	return LOG_INFO;
	}
	return LOG_DEBUG;
}
#endif

static char *lutil_levels[] = {"emergency", "alert", "critical",
			   "error", "warning", "notice",
			   "information", "entry", "args",
			   "results", "detail1", "detail2",
			   NULL};

static char *lutil_subsys[LDAP_SUBSYS_NUM] = {"global","operation", "transport",
			   	"connection", "filter", "ber", 
				"config", "acl", "cache", "index", 
				"ldif", "tools", "slapd", "slurpd",
				"backend", "back_bdb", "back_ldbm", 
				"back_ldap", "back_meta", "back_mon" };

int lutil_mnem2subsys( const char *subsys )
{
    int i;
    for( i = 0; i < LDAP_SUBSYS_NUM; i++ )
    {
		if ( !strcasecmp( subsys, lutil_subsys[i] ) )
		{
	    	return i;
		}
    }
    return -1;
}

void lutil_set_all_backends( int level )
{
    int i;

    for( i = 0; i < LDAP_SUBSYS_NUM; i++ )
    {
		if ( !strncasecmp( "back_", lutil_subsys[i], strlen("back_") ) )
		{
			ldap_loglevels[i] = level;
		}
    }
}

int lutil_mnem2level( const char *level )
{
    int i;
    for( i = 0; lutil_levels[i] != NULL; i++ )
    {
	if ( !strcasecmp( level, lutil_levels[i] ) )
	{
	    return i;
	}
    }
    return -1;
}

static int addSubsys( const char *subsys, int level )
{
	int subsys_num;

	if ( !strcasecmp( subsys, "backend" ) )
	{
		lutil_set_all_backends( level );
		return level;
	}
	else
	{
		subsys_num = lutil_mnem2subsys(subsys);
		if(subsys_num < 0)
		{
			fprintf(stderr, "Unknown Subsystem name [ %s ] - Discarded\n", 
				subsys);
			fflush(stderr);
			return -1;
		}

		ldap_loglevels[subsys_num] = level;
		return level;
	}
	return -1;
}

int lutil_set_debug_level( const char* subsys, int level )
{
    return( addSubsys( subsys, level ) );
}

int lutil_debug_file( FILE *file )
{
	log_file = file;
	ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, file );

	return 0;
}

void lutil_log_int(
	FILE* file,
	const char *subsys, int level,
	const char *fmt, va_list vl )
{
#ifdef HAVE_WINSOCK
	time_t now;
	struct tm *today;
#endif
	size_t i;
	char * t_subsys;
	char * tmp;

	t_subsys = strdup(subsys);
	
	for(tmp = t_subsys, i = 0; i < strlen(t_subsys); i++, tmp++)
		*tmp = TOUPPER( (unsigned char) *tmp );

#ifdef LDAP_SYSLOG
	/* we're configured to use syslog */
	if( use_syslog ) {
#ifdef HAVE_VSYSLOG
		vsyslog( debug2syslog(level), fmt, vl );
#else
		char data[4096];
		vsnprintf( data, sizeof(data), fmt, vl );
		syslog( debug2syslog(level), data );
#endif
		return;
	}
#endif

#if 0
#ifdef HAVE_WINSOCK
	if( log_file == NULL ) {
		log_file = fopen( LDAP_RUNDIR LDAP_DIRSEP "openldap.log", "w" );

		if ( log_file == NULL )
			log_file = fopen( "openldap.log", "w" );

		if ( log_file == NULL )
			return;

		ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, log_file );
	}
#endif
#endif

	if( file == NULL ) {
		/*
		 * Use stderr unless file was specified via:
		 *   ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, file)
		 */
		file = stderr;
	}

#ifdef HAVE_WINSOCK
	/*
	 * Stick the time in the buffer to output when using Winsock
	 * as NT can't pipe to a timestamp program like Unix can.
	 * This, of course, makes some logs hard to read.
     */
	time( &now );
	today = localtime( &now );
	fprintf( file, "%4d%02d%02d:%02d:%02d:%02d ",
		today->tm_year + 1900, today->tm_mon + 1,
		today->tm_mday, today->tm_hour,
		today->tm_min, today->tm_sec );
#endif

	/*
	 * format the output data.
	 */

	fprintf(file, "\n%s:: ", t_subsys ); 
	vfprintf( file, fmt, vl );
	fflush( file );
}

/*
 * The primary logging routine.	 Takes the subsystem being logged from, the
 * level of the log output and the format and data.  Send this on to the
 * internal routine with the print file, if any.
 */
void lutil_log( const int subsys, int level, const char *fmt, ... )
{
	FILE* outfile = NULL;
	va_list vl;
	va_start( vl, fmt );
	ber_get_option( NULL, LBER_OPT_LOG_PRINT_FILE, &outfile );
	lutil_log_int( outfile, lutil_subsys[subsys], level, fmt, vl );
	va_end( vl );
}

void lutil_log_initialize(int argc, char **argv)
{
    int i;
    /*
     * Start by setting the hook for the libraries to use this logging
     * routine.
     */
    ber_set_option( NULL, LBER_OPT_LOG_PROC, (void*)lutil_log_int );

    if ( argc == 0 ) return;
    /*
     * Now go through the command line options to set the debugging
     * levels
     */
    for( i = 0; i < argc; i++ )
    {
		char *next = argv[i];
	
		if ( i < argc-1 && next[0] == '-' && next[1] == 'd' )
		{
	   		char subsys[64];
	   		int level;
	    	char *optarg = argv[i+1];
	    	char *index = strchr( optarg, '=' );
	    	if ( index != NULL )
	    	{
				*index = 0;
				strcpy ( subsys, optarg );
				level = atoi( index+1 );
				if ( level <= 0 ) level = lutil_mnem2level( index + 1 );
				lutil_set_debug_level( subsys, level );
				*index = '=';
	    	}
	    	else
	    	{
				global_level = atoi( optarg );
				ldap_loglevels[0] = global_level;
				/* 
		 		* if a negative number was used, make the global level the
		 		* maximum sane level.
		 		*/
				if ( global_level < 0 ) 
				{
					global_level = 65535;
					ldap_loglevels[0] = 65535;
	    		}
	    	}
		}
    }
}

void (lutil_debug)( int debug, int level, const char *fmt, ... )
{
	char buffer[4096];
	va_list vl;

	if ( !(level & debug ) )
		return;

#ifdef HAVE_WINSOCK
	if( log_file == NULL ) {
		log_file = fopen( LDAP_RUNDIR LDAP_DIRSEP "openldap.log", "w" );

		if ( log_file == NULL )
			log_file = fopen( "openldap.log", "w" );

		if ( log_file == NULL )
			return;

		ber_set_option( NULL, LBER_OPT_LOG_PRINT_FILE, log_file );
	}
#endif
	va_start( vl, fmt );

	vsnprintf( buffer, sizeof(buffer), fmt, vl );
	buffer[sizeof(buffer)-1] = '\0';

	if( log_file != NULL ) {
		fputs( buffer, log_file );
		fflush( log_file );
	}

	fputs( buffer, stderr );
	va_end( vl );
}
