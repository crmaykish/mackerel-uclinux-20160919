/* line64.c - routines for dealing with the slapd line format */
/* $OpenLDAP: pkg/ldap/libraries/libldif/fetch.c,v 1.12.2.2 2003/03/03 17:10:06 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/time.h>

#ifdef HAVE_FETCH
#include <fetch.h>
#endif

#include "ldap_log.h"
#include "lber_pvt.h"
#include "ldap_pvt.h"
#include "ldap_config.h"
#include "ldif.h"

int
ldif_fetch_url(
    LDAP_CONST char	*urlstr,
    char	**valuep,
    ber_len_t *vlenp
)
{
	FILE *url;
	char buffer[1024];
	char *p = NULL;
	size_t total;
	size_t bytes;

	*valuep = NULL;
	*vlenp = 0;

#ifdef HAVE_FETCH
	url = fetchGetURL( (char*) urlstr, "" );

#else
	if( strncasecmp( "file://", urlstr, sizeof("file://")-1 ) == 0 ) {
		p = strchr( &urlstr[sizeof("file://")-1], '/' );
		if( p == NULL ) {
			return -1;
		}

		/* we don't check for LDAP_DIRSEP since URLs should contain '/' */
		if( *p != '/' ) {
			/* skip over false root */
			p++;
		}

		p = ber_strdup( p );
		ldap_pvt_hex_unescape( p );

		url = fopen( p, "rb" );

	} else {
		return -1;
	}
#endif

	if( url == NULL ) {
		return -1;
	}

	total = 0;

	while( (bytes = fread( buffer, 1, sizeof(buffer), url )) != 0 ) {
		char *newp = ber_memrealloc( p, total + bytes + 1 );
		if( newp == NULL ) {
			ber_memfree( p );
			fclose( url );
			return -1;
		}
		p = newp;
		AC_MEMCPY( &p[total], buffer, bytes );
		total += bytes;
	}

	fclose( url );

	if( total == 0 ) {
		char *newp = ber_memrealloc( p, 1 );
		if( newp == NULL ) {
			ber_memfree( p );
			return -1;
		}
		p = newp;
	}

	p[total] = '\0';
	*valuep = p;
	*vlenp = total;

	return 0;
}
