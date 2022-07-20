/* $OpenLDAP: pkg/ldap/libraries/libldap/messages.c,v 1.9.2.2 2003/03/03 17:10:05 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*
 *  messages.c
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

LDAPMessage *
ldap_first_message( LDAP *ld, LDAPMessage *chain )
{
	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( chain != NULL );

  	return chain;
}

LDAPMessage *
ldap_next_message( LDAP *ld, LDAPMessage *msg )
{
	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );
	assert( msg != NULL );

	return msg->lm_chain;
}

int
ldap_count_messages( LDAP *ld, LDAPMessage *chain )
{
	int	i;

	assert( ld != NULL );
	assert( LDAP_VALID( ld ) );

	for ( i = 0; chain != NULL; chain = chain->lm_chain ) {
		i++;
	}

	return( i );
}
