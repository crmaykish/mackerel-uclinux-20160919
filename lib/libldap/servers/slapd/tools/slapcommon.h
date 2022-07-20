/* $OpenLDAP: pkg/ldap/servers/slapd/tools/slapcommon.h,v 1.11.2.1 2003/03/03 17:10:11 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */
/* slapcommon.h - common definitions for the slap tools */

#ifndef SLAPCOMMON_H_
#define SLAPCOMMON_H_ 1

#define SLAPD_TOOLS 1
#include "../slap.h"

enum slaptool {
	SLAPCAT=1,	/* database -> LDIF tool */
	SLAPADD,	/* LDIF -> database tool */
	SLAPINDEX,	/* database index tool */
	SLAPTEST	/* database testing tool */
};


extern	char	*progname;
extern	char	*conffile;
extern	Backend *be;
extern	int		appendmode;
extern	int		verbose;
extern	int		continuemode;
extern	int		nosubordinates;
extern	int		dryrun;

extern	char	*ldiffile;
extern	FILE	*ldiffp;

void slap_tool_init LDAP_P((
	const char* name,
	int tool,
	int argc, char **argv ));

void slap_tool_destroy LDAP_P((void));

#endif /* SLAPCOMMON_H_ */
