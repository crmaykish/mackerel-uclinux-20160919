/* init.c - initialize ldap backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-dnssrv/init.c,v 1.14.2.2 2003/03/03 17:10:09 kurt Exp $ */
/*
 * Copyright 2000-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"
#include "external.h"

#ifdef SLAPD_DNSSRV_DYNAMIC

int back_dnssrv_LTX_init_module(int argc, char *argv[])
{
    BackendInfo bi;

    memset( &bi, '\0', sizeof(bi) );
    bi.bi_type = "dnssrv";
    bi.bi_init = dnssrv_back_initialize;

    backend_add( &bi );
    return 0;
}

#endif /* SLAPD_DNSSRV_DYNAMIC */

int
dnssrv_back_initialize(
    BackendInfo	*bi )
{
	static char *controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
 		LDAP_CONTROL_VALUESRETURNFILTER,
		NULL
	};

	bi->bi_controls = controls;

	bi->bi_open = 0;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = 0;
	bi->bi_db_destroy = 0;
	bi->bi_db_config = dnssrv_back_db_config;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;

	bi->bi_chk_referrals = dnssrv_back_referrals;

	bi->bi_op_bind = dnssrv_back_bind;
	bi->bi_op_search = dnssrv_back_search;
	bi->bi_op_compare = 0 /* dnssrv_back_compare */;
	bi->bi_op_modify = 0;
	bi->bi_op_modrdn = 0;
	bi->bi_op_add = 0;
	bi->bi_op_delete = 0;
	bi->bi_op_abandon = 0;
	bi->bi_op_unbind = 0;

	bi->bi_extended = 0;
	bi->bi_acl_group = 0;
	bi->bi_acl_attribute = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return 0;
}

int
dnssrv_back_db_init(
    Backend	*be )
{
	return 0;
}

int
dnssrv_back_db_destroy(
    Backend	*be )
{
	return 0;
}
