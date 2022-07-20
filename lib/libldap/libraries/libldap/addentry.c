/* $OpenLDAP: pkg/ldap/libraries/libldap/addentry.c,v 1.10.2.1 2003/03/03 17:10:04 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/*  Portions
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  addentry.c
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

LDAPMessage *
ldap_delete_result_entry( LDAPMessage **list, LDAPMessage *e )
{
	LDAPMessage	*tmp, *prev = NULL;

	assert( list != NULL );
	assert( e != NULL );

	for ( tmp = *list; tmp != NULL && tmp != e; tmp = tmp->lm_chain )
		prev = tmp;

	if ( tmp == NULL )
		return( NULL );

	if ( prev == NULL )
		*list = tmp->lm_chain;
	else
		prev->lm_chain = tmp->lm_chain;
	tmp->lm_chain = NULL;

	return( tmp );
}

void
ldap_add_result_entry( LDAPMessage **list, LDAPMessage *e )
{
	assert( list != NULL );
	assert( e != NULL );

	e->lm_chain = *list;
	*list = e;
}
