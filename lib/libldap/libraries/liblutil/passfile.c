/* $OpenLDAP: pkg/ldap/libraries/liblutil/passfile.c,v 1.1.2.3 2003/03/03 17:10:06 kurt Exp $ */
/*
 * Copyright 2002-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/string.h>

#ifdef HAVE_FSTAT
#include <sys/types.h>
#include <sys/stat.h>
#endif /* HAVE_FSTAT */

#include <lber.h>
#include <lutil.h>

/* Get a password from a file. */
int
lutil_get_filed_password(
	const char *filename,
	struct berval *passwd )
{
	size_t nread, nleft, nr;
	FILE *f = fopen( filename, "r" );

	if( f == NULL ) {
		perror( filename );
		return -1;
	}

	passwd->bv_val = NULL;
	passwd->bv_len = 4196;

#ifdef HAVE_FSTAT
	{
		struct stat sb;
		if ( fstat( fileno( f ), &sb ) == 0 ) {
			if( sb.st_mode & 006 ) {
				fprintf( stderr,
					"Warning: Password file %s is publicly readable/writeable\n",
					filename );
			}

			passwd->bv_len = sb.st_size;
		}
	}
#endif /* HAVE_FSTAT */

	passwd->bv_val = (char *) malloc( passwd->bv_len + 1 );
	if( passwd->bv_val == NULL ) {
		perror( filename );
		return -1;
	}

	nread = 0;
	nleft = passwd->bv_len;
	do {
		if( nleft == 0 ) {
			/* double the buffer size */
			char *p = (char *) realloc( passwd->bv_val,
				2 * passwd->bv_len + 1 );
			if( p == NULL ) {
				free( passwd->bv_val );
				passwd->bv_val = NULL;
				passwd->bv_len = 0;
				return -1;
			}
			nleft = passwd->bv_len;
			passwd->bv_len *= 2;
			passwd->bv_val = p;
		}

		nr = fread( &passwd->bv_val[nread], 1, nleft, f );

		if( nr < nleft && ferror( f ) ) {
			free( passwd->bv_val );
			passwd->bv_val = NULL;
			passwd->bv_len = 0;
			return -1;
		}

		nread += nr;
		nleft -= nr;
	} while ( !feof(f) );

	passwd->bv_len = nread;
	passwd->bv_val[nread] = '\0';

	fclose( f );
	return 0;
}
