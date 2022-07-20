/* $OpenLDAP: pkg/ldap/libraries/libldap/getvalues.c,v 1.15.2.6 2003/03/03 17:10:04 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*  Portions
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  getvalues.c
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

char **
ldap_get_values( LDAP *ld, LDAPMessage *entry, LDAP_CONST char *target )
{
	BerElement	ber;
	char		*attr;
	int		found = 0;
	char		**vals;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( entry != NULL );
	assert( target != NULL );

#ifdef NEW_LOGGING
	LDAP_LOG ( OPERATION, ENTRY, "ldap_get_values\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_get_values\n", 0, 0, 0 );
#endif

	ber = *entry->lm_ber;

	/* skip sequence, dn, sequence of, and snag the first attr */
	if ( ber_scanf( &ber, "{x{{a" /*}}}*/, &attr ) == LBER_ERROR ) {
		ld->ld_errno = LDAP_DECODING_ERROR;
		return( NULL );
	}

	if ( strcasecmp( target, attr ) == 0 )
		found = 1;

	/* break out on success, return out on error */
	while ( ! found ) {
		LDAP_FREE(attr);
		attr = NULL;

		if ( ber_scanf( &ber, /*{*/ "x}{a" /*}*/, &attr ) == LBER_ERROR ) {
			ld->ld_errno = LDAP_DECODING_ERROR;
			return( NULL );
		}

		if ( strcasecmp( target, attr ) == 0 )
			break;

	}

	LDAP_FREE(attr);
	attr = NULL;

	/* 
	 * if we get this far, we've found the attribute and are sitting
	 * just before the set of values.
	 */

	if ( ber_scanf( &ber, "[v]", &vals ) == LBER_ERROR ) {
		ld->ld_errno = LDAP_DECODING_ERROR;
		return( NULL );
	}

	return( vals );
}

struct berval **
ldap_get_values_len( LDAP *ld, LDAPMessage *entry, LDAP_CONST char *target )
{
	BerElement	ber;
	char		*attr;
	int		found = 0;
	struct berval	**vals;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( entry != NULL );
	assert( target != NULL );

#ifdef NEW_LOGGING
	LDAP_LOG ( OPERATION, ENTRY, "ldap_get_values_len\n", 0, 0, 0 );
#else
	Debug( LDAP_DEBUG_TRACE, "ldap_get_values_len\n", 0, 0, 0 );
#endif

	ber = *entry->lm_ber;

	/* skip sequence, dn, sequence of, and snag the first attr */
	if ( ber_scanf( &ber, "{x{{a" /* }}} */, &attr ) == LBER_ERROR ) {
		ld->ld_errno = LDAP_DECODING_ERROR;
		return( NULL );
	}

	if ( strcasecmp( target, attr ) == 0 )
		found = 1;

	/* break out on success, return out on error */
	while ( ! found ) {
		LDAP_FREE( attr );
		attr = NULL;

		if ( ber_scanf( &ber, /*{*/ "x}{a" /*}*/, &attr ) == LBER_ERROR ) {
			ld->ld_errno = LDAP_DECODING_ERROR;
			return( NULL );
		}

		if ( strcasecmp( target, attr ) == 0 )
			break;
	}

	LDAP_FREE( attr );
	attr = NULL;

	/* 
	 * if we get this far, we've found the attribute and are sitting
	 * just before the set of values.
	 */

	if ( ber_scanf( &ber, "[V]", &vals ) == LBER_ERROR ) {
		ld->ld_errno = LDAP_DECODING_ERROR;
		return( NULL );
	}

	return( vals );
}

int
ldap_count_values( char **vals )
{
	int	i;

	if ( vals == NULL )
		return( 0 );

	for ( i = 0; vals[i] != NULL; i++ )
		;	/* NULL */

	return( i );
}

int
ldap_count_values_len( struct berval **vals )
{
	return( ldap_count_values( (char **) vals ) );
}

void
ldap_value_free( char **vals )
{
	LDAP_VFREE( vals );
}

void
ldap_value_free_len( struct berval **vals )
{
	ber_bvecfree( vals );
}

char **
ldap_value_dup( char *const *vals )
{
	char **new;
	int i;

	if( vals == NULL ) {
		return NULL;
	}

	for( i=0; vals[i]; i++ ) {
		;   /* Count the number of values */
	}

	if( i == 0 ) {
		return NULL;
	}

	new = LDAP_MALLOC( (i+1)*sizeof(char *) );  /* Alloc array of pointers */
	if( new == NULL ) {
		return NULL;
	}

	for( i=0; vals[i]; i++ ) {
		new[i] = LDAP_STRDUP( vals[i] );   /* Dup each value */
		if( new[i] == NULL ) {
			LDAP_VFREE( new );
			return NULL;
		}
	}
	new[i] = NULL;

	return new;
}

