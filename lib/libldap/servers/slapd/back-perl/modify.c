/* $OpenLDAP: pkg/ldap/servers/slapd/back-perl/modify.c,v 1.8.2.3 2002/06/20 20:12:34 kurt Exp $ */
/*
 *	 Copyright 1999, John C. Quillan, All rights reserved.
 *	 Portions Copyright 2002, myinternet Limited. All rights reserved.
 *
 *	 Redistribution and use in source and binary forms are permitted only
 *	 as authorized by the OpenLDAP Public License.	A copy of this
 *	 license is available at http://www.OpenLDAP.org/license.html or
 *	 in file LICENSE in the top-level directory of the distribution.
 */

#include "portable.h"

#include <stdio.h>

#include "slap.h"
#ifdef HAVE_WIN32_ASPERL
#include "asperl_undefs.h"
#endif

#include <EXTERN.h>
#include <perl.h>

#include "perl_back.h"

int
perl_back_modify(
	Backend	*be,
	Connection	*conn,
	Operation	*op,
	struct berval 	*dn,
	struct berval 	*ndn,
	Modifications	*modlist
)
{
	char test[500];
	int return_code;
	int count;
	int i;
	int err = 0;
	char *matched = NULL, *info = NULL;

	PerlBackend *perl_back = (PerlBackend *)be->be_private;

	ldap_pvt_thread_mutex_lock( &perl_interpreter_mutex );	

	{
		dSP; ENTER; SAVETMPS;
		
		PUSHMARK(sp);
		XPUSHs( perl_back->pb_obj_ref );
		XPUSHs(sv_2mortal(newSVpv( dn->bv_val , 0)));

		for (; modlist != NULL; modlist = modlist->sml_next ) {
			Modification *mods = &modlist->sml_mod;

			switch ( mods->sm_op & ~LDAP_MOD_BVALUES ) {
			case LDAP_MOD_ADD:
				XPUSHs(sv_2mortal(newSVpv("ADD", 0 )));
				break;
				
			case LDAP_MOD_DELETE:
				XPUSHs(sv_2mortal(newSVpv("DELETE", 0 )));
				break;
				
			case LDAP_MOD_REPLACE:
				XPUSHs(sv_2mortal(newSVpv("REPLACE", 0 )));
				break;
			}

			
			XPUSHs(sv_2mortal(newSVpv( mods->sm_desc->ad_cname.bv_val, 0 )));

			for ( i = 0;
				mods->sm_bvalues != NULL && mods->sm_bvalues[i].bv_val != NULL;
				i++ )
			{
				XPUSHs(sv_2mortal(newSVpv( mods->sm_bvalues[i].bv_val, 0 )));
			}
		}

		PUTBACK;

#ifdef PERL_IS_5_6
		count = call_method("modify", G_SCALAR);
#else
		count = perl_call_method("modify", G_SCALAR);
#endif

		SPAGAIN;

		if (count != 1) {
			croak("Big trouble in back_modify\n");
		}
							 
		return_code = POPi;

		PUTBACK; FREETMPS; LEAVE;
	}

	ldap_pvt_thread_mutex_unlock( &perl_interpreter_mutex );

	send_ldap_result( conn, op, return_code,
		NULL, NULL, NULL, NULL );

	Debug( LDAP_DEBUG_ANY, "Perl MODIFY\n", 0, 0, 0 );
	return( 0 );
}

