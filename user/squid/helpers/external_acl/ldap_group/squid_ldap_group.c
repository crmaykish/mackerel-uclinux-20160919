/*
 * squid_ldap_group: lookup group membership in LDAP
 *
 * (C)2002,2003 MARA Systems AB
 *
 * License: squid_ldap_group is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 * 
 * Authors:
 *  Flavio Pescuma <flavio@marasystems.com>
 *  Henrik Nordstrom <hno@marasystems.com>
 *  MARA Systems AB, Sweden <http://www.marasystems.com>
 *
 * With contributions from others mentioned in the ChangeLog file
 *
 * In part based on squid_ldap_auth by Glen Newton and Henrik Nordstrom.
 *
 * Latest version of this program can always be found from MARA Systems
 * at http://marasystems.com/download/LDAP_Group/
 * 
 * Dependencies: You need to get the OpenLDAP libraries
 * from http://www.openldap.org or use another compatible
 * LDAP C-API library.
 *
 * If you want to make a TLS enabled connection you will also need the
 * OpenSSL libraries linked into openldap. See http://www.openssl.org/
 * 
 * License: squid_ldap_group is free software; you can redistribute it 
 * and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2, 
 * or (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <lber.h>
#include <ldap.h>
#if defined(LDAP_OPT_NETWORK_TIMEOUT)
#include <sys/time.h>
#endif

#define PROGRAM_NAME "squid_ldap_group"
#define VERSION "2.17-2.5"

/* Globals */

static char *basedn = NULL;
static char *searchfilter = NULL;
static char *userbasedn = NULL;
static char *userdnattr = NULL;
static char *usersearchfilter = NULL;
static char *binddn = NULL;
static char *bindpasswd = NULL;
static int searchscope = LDAP_SCOPE_SUBTREE;
static int persistent = 0;
static int noreferrals = 0;
static int debug = 0;
static int aliasderef = LDAP_DEREF_NEVER;
#if defined(NETSCAPE_SSL)
static char *sslpath = NULL;
static int sslinit = 0;
#endif
static int connect_timeout = 0;
static int timelimit = LDAP_NO_LIMIT;

#ifdef LDAP_VERSION3
/* Added for TLS support and version 3 */
static int use_tls = 0;
static int version = -1;
#endif

static int searchLDAP(LDAP * ld, char *group, char *user, char *extension_dn);

static int readSecret(char *filename);

/* Yuck.. we need to glue to different versions of the API */

#ifndef LDAP_NO_ATTRS
#define LDAP_NO_ATTRS "1.1"
#endif

#if defined(LDAP_API_VERSION) && LDAP_API_VERSION > 1823
static int 
squid_ldap_errno(LDAP * ld)
{
    int err = 0;
    ldap_get_option(ld, LDAP_OPT_ERROR_NUMBER, &err);
    return err;
}
static void 
squid_ldap_set_aliasderef(LDAP * ld, int deref)
{
    ldap_set_option(ld, LDAP_OPT_DEREF, &deref);
}
static void 
squid_ldap_set_referrals(LDAP * ld, int referrals)
{
    int *value = referrals ? LDAP_OPT_ON : LDAP_OPT_OFF;
    ldap_set_option(ld, LDAP_OPT_REFERRALS, value);
}
static void
squid_ldap_set_timelimit(LDAP *ld, int timelimit)
{
    ldap_set_option(ld, LDAP_OPT_TIMELIMIT, &timelimit);
}
static void
squid_ldap_set_connect_timeout(LDAP *ld, int timelimit)
{
#if defined(LDAP_OPT_NETWORK_TIMEOUT)
    struct timeval tv;
    tv.tv_sec = timelimit;
    tv.tv_usec = 0;
    ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &tv);
#elif defined(LDAP_X_OPT_CONNECT_TIMEOUT)
    timelimit *= 1000;
    ldap_set_option(ld, LDAP_X_OPT_CONNECT_TIMEOUT, &timelimit);
#endif
}
static void 
squid_ldap_memfree(char *p)
{
    ldap_memfree(p);
}
#else
static int 
squid_ldap_errno(LDAP * ld)
{
    return ld->ld_errno;
}
static void 
squid_ldap_set_aliasderef(LDAP * ld, int deref)
{
    ld->ld_deref = deref;
}
static void 
squid_ldap_set_referrals(LDAP * ld, int referrals)
{
    if (referrals)
	ld->ld_options |= ~LDAP_OPT_REFERRALS;
    else
	ld->ld_options &= ~LDAP_OPT_REFERRALS;
}
static void
squid_ldap_set_timelimit(LDAP *ld, int timelimit)
{
    ld->ld_timelimit = timelimit;
}
static void
squid_ldap_set_connect_timeout(LDAP *ld, int timelimit)
{
    fprintf(stderr, "Connect timeouts not supported in your LDAP library\n");
}
static void 
squid_ldap_memfree(char *p)
{
    free(p);
}
#endif

#ifdef LDAP_API_FEATURE_X_OPENLDAP
  #if LDAP_VENDOR_VERSION > 194
    #define HAS_URI_SUPPORT 1
  #endif
#endif

static char *
strwordtok(char *buf, char **t)
{
    unsigned char *word = NULL;
    unsigned char *p = (unsigned char *) buf;
    unsigned char *d;
    unsigned char ch;
    int quoted = 0;
    if (!p)
	p = (unsigned char *) *t;
    if (!p)
	goto error;
    while (*p && isspace(*p))
	p++;
    if (!*p)
	goto error;
    word = d = p;
    while ((ch = *p)) {
	switch (ch) {
	case '\\':
	    p++;
	    *d++ = ch = *p;
	    if (ch)
		p++;
	    break;
	case '"':
	    quoted = !quoted;
	    p++;
	    break;
	default:
	    if (!quoted && isspace(*p)) {
		p++;
		goto done;
	    }
	    *d++ = *p++;
	    break;
	}
    }
  done:
    *d++ = '\0';
  error:
    *t = (char *) p;
    return (char *) word;
}

int
main(int argc, char **argv)
{
    char buf[8192];
    char *user, *group, *extension_dn = NULL;
    char *ldapServer = NULL;
    LDAP *ld = NULL;
    int tryagain = 0, rc;
    int port = LDAP_PORT;
    int use_extension_dn = 0;
    int strip_nt_domain = 0;

    setbuf(stdout, NULL);

    while (argc > 1 && argv[1][0] == '-') {
	char *value = "";
	char option = argv[1][1];
	switch (option) {
	case 'P':
	case 'R':
	case 'z':
	case 'Z':
	case 'd':
	case 'g':
	case 'S':
	    break;
	default:
	    if (strlen(argv[1]) > 2) {
		value = argv[1] + 2;
	    } else if (argc > 2) {
		value = argv[2];
		argv++;
		argc--;
	    } else
		value = "";
	    break;
	}
	argv++;
	argc--;
	switch (option) {
	case 'H':
#if !HAS_URI_SUPPORT
	    fprintf(stderr, "ERROR: Your LDAP library does not have URI support\n");
	    exit(1);
#endif
	    /* Fall thru to -h */
	case 'h':
	    if (ldapServer) {
		int len = strlen(ldapServer) + 1 + strlen(value) + 1;
		char *newhost = malloc(len);
		snprintf(newhost, len, "%s %s", ldapServer, value);
		free(ldapServer);
		ldapServer = newhost;
	    } else {
		ldapServer = strdup(value);
	    }
	    break;
	case 'b':
	    basedn = value;
	    break;
	case 'f':
	    searchfilter = value;
	    break;
	case 'B':
	    userbasedn = value;
	    break;
	case 'F':
	    usersearchfilter = value;
	    break;
	case 'u':
	    userdnattr = value;
	    break;
	case 's':
	    if (strcmp(value, "base") == 0)
		searchscope = LDAP_SCOPE_BASE;
	    else if (strcmp(value, "one") == 0)
		searchscope = LDAP_SCOPE_ONELEVEL;
	    else if (strcmp(value, "sub") == 0)
		searchscope = LDAP_SCOPE_SUBTREE;
	    else {
		fprintf(stderr, PROGRAM_NAME " ERROR: Unknown search scope '%s'\n", value);
		exit(1);
	    }
	    break;
	case 'E':
#if defined(NETSCAPE_SSL)
	    sslpath = value;
	    if (port == LDAP_PORT)
		port = LDAPS_PORT;
#else
	    fprintf(stderr, PROGRAM_NAME " ERROR: -E unsupported with this LDAP library\n");
	    exit(1);
#endif
	    break;
	case 'c':
	    connect_timeout = atoi(value);
	    break;
	case 't':
	    timelimit = atoi(value);
	    break;
	case 'a':
	    if (strcmp(value, "never") == 0)
		aliasderef = LDAP_DEREF_NEVER;
	    else if (strcmp(value, "always") == 0)
		aliasderef = LDAP_DEREF_ALWAYS;
	    else if (strcmp(value, "search") == 0)
		aliasderef = LDAP_DEREF_SEARCHING;
	    else if (strcmp(value, "find") == 0)
		aliasderef = LDAP_DEREF_FINDING;
	    else {
		fprintf(stderr, PROGRAM_NAME " ERROR: Unknown alias dereference method '%s'\n", value);
		exit(1);
	    }
	    break;
	case 'D':
	    binddn = value;
	    break;
	case 'w':
	    bindpasswd = value;
	    break;
	case 'W':
	    readSecret (value);
	    break;
	case 'P':
	    persistent = !persistent;
	    break;
	case 'p':
	    port = atoi(value);
	    break;
	case 'R':
	    noreferrals = !noreferrals;
	    break;
#ifdef LDAP_VERSION3
	case 'v':
	    switch (atoi(value)) {
	    case 2:
		version = LDAP_VERSION2;
		break;
	    case 3:
		version = LDAP_VERSION3;
		break;
	    default:
		fprintf(stderr, "Protocol version should be 2 or 3\n");
		exit(1);
	    }
	    break;
	case 'Z':
	    if (version == LDAP_VERSION2) {
		fprintf(stderr, "TLS (-Z) is incompatible with version %d\n",
		    version);
		exit(1);
	    }
	    version = LDAP_VERSION3;
	    use_tls = 1;
	    break;
#endif
	case 'd':
	    debug = 1;
	    break;
	case 'g':
	    use_extension_dn = 1;
	    break;
	case 'S':
	    strip_nt_domain = 1;
	    break;
	default:
	    fprintf(stderr, PROGRAM_NAME " ERROR: Unknown command line option '%c'\n", option);
	    exit(1);
	}
    }

    while (argc > 1) {
	char *value = argv[1];
	if (ldapServer) {
	    int len = strlen(ldapServer) + 1 + strlen(value) + 1;
	    char *newhost = malloc(len);
	    snprintf(newhost, len, "%s %s", ldapServer, value);
	    free(ldapServer);
	    ldapServer = newhost;
	} else {
	    ldapServer = strdup(value);
	}
	argc--;
	argv++;
    }

    if (!ldapServer)
	ldapServer = "localhost";

    if (!basedn || !searchfilter) {
	fprintf(stderr, "\n" PROGRAM_NAME " version " VERSION "\n\n");
	fprintf(stderr, "Usage: " PROGRAM_NAME " -b basedn -f filter [options] ldap_server_name\n\n");
	fprintf(stderr, "\t-b basedn (REQUIRED)\tbase dn under where to search for groups\n");
	fprintf(stderr, "\t-f filter (REQUIRED)\tgroup search filter pattern. %%v = user,\n\t\t\t\t%%a = group\n");
	fprintf(stderr, "\t-B basedn (REQUIRED)\tbase dn under where to search for users\n");
	fprintf(stderr, "\t-F filter (REQUIRED)\tuser search filter pattern. %%s = login\n");
	fprintf(stderr, "\t-s base|one|sub\t\tsearch scope\n");
	fprintf(stderr, "\t-D binddn\t\tDN to bind as to perform searches\n");
	fprintf(stderr, "\t-w bindpasswd\t\tpassword for binddn\n");
	fprintf(stderr, "\t-W secretfile\t\tread password for binddn from file secretfile\n");
#if HAS_URI_SUPPORT
	fprintf(stderr, "\t-H URI\t\t\tLDAPURI (defaults to ldap://localhost)\n");
#endif
	fprintf(stderr, "\t-h server\t\tLDAP server (defaults to localhost)\n");
	fprintf(stderr, "\t-p port\t\t\tLDAP server port (defaults to %d)\n", LDAP_PORT);
	fprintf(stderr, "\t-P\t\t\tpersistent LDAP connection\n");
#if defined(NETSCAPE_SSL)
	fprintf(stderr, "\t-E sslcertpath\t\tenable LDAP over SSL\n");
#endif
	fprintf(stderr, "\t-c timeout\t\tconnect timeout\n");
	fprintf(stderr, "\t-t timelimit\t\tsearch time limit\n");
	fprintf(stderr, "\t-R\t\t\tdo not follow referrals\n");
	fprintf(stderr, "\t-a never|always|search|find\n\t\t\t\twhen to dereference aliases\n");
#ifdef LDAP_VERSION3
	fprintf(stderr, "\t-v 2|3\t\t\tLDAP version\n");
	fprintf(stderr, "\t-Z\t\t\tTLS encrypt the LDAP connection, requires\n\t\t\t\tLDAP version 3\n");
#endif
	fprintf(stderr, "\t-g\t\t\tfirst query parameter is base DN extension\n\t\t\t\tfor this query\n");
	fprintf(stderr, "\t-S\t\t\tStrip NT domain from usernames\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\tIf you need to bind as a user to perform searches then use the\n\t-D binddn -w bindpasswd or -D binddn -W secretfile options\n\n");
	exit(1);
    }
    while (fgets(buf, 256, stdin) != NULL) {
	char *tptr;
	int found = 0;
	user = strwordtok(buf, &tptr);
	if (user && strip_nt_domain) {
	    char *u = strchr(user, '\\');
	    if (!u)
		u = strchr(user, '/');
	    if (u && u[1])
		user = u + 1;
	}
	if (use_extension_dn)
		extension_dn = strwordtok(NULL, &tptr);

	while (!found && user && (group = strwordtok(NULL, &tptr)) != NULL) {

	  recover:
	    if (ld == NULL) {
#if HAS_URI_SUPPORT
	    	if (strstr(ldapServer, "://") != NULL) {
		    rc = ldap_initialize( &ld, ldapServer );
		    if( rc != LDAP_SUCCESS ) {
			fprintf(stderr, "\nUnable to connect to LDAPURI:%s\n", ldapServer);
			break;
		    }
	    	} else
#endif
#if NETSCAPE_SSL
		if (sslpath) {
		    if ( !sslinit && (ldapssl_client_init(sslpath, NULL) != LDAP_SUCCESS)) {
			fprintf(stderr, "\nUnable to initialise SSL with cert path %s\n",
				sslpath);
			exit(1);
		    } else {
			sslinit++;
		    }
		    if ((ld = ldapssl_init(ldapServer, port, 1)) == NULL) {
			fprintf(stderr, "\nUnable to connect to SSL LDAP server: %s port:%d\n",
				ldapServer, port);
			exit(1);
		    }
		} else
#endif
		if ((ld = ldap_init(ldapServer, port)) == NULL) {
		    fprintf(stderr, "\nUnable to connect to LDAP server:%s port:%d\n",ldapServer, port);
		    break;
		}

		if (connect_timeout)
		    squid_ldap_set_connect_timeout(ld, connect_timeout);

#ifdef LDAP_VERSION3
		if (version == -1) {
		    version = LDAP_VERSION2;
		}
		if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version) != LDAP_SUCCESS) {
		    fprintf(stderr, "Could not set LDAP_OPT_PROTOCOL_VERSION %d\n",
			version);
		    ldap_unbind(ld);
		    ld = NULL;
		    break;
		}
		if (use_tls) {
#ifdef LDAP_OPT_X_TLS
		    if (version == LDAP_VERSION3 && ldap_start_tls_s(ld, NULL, NULL) != LDAP_SUCCESS) {
			fprintf(stderr, "Could not Activate TLS connection\n");
			ldap_unbind(ld);
			ld = NULL;
			break;
		    } else {
			fprintf(stderr, "TLS requires LDAP version 3\n");
			exit(1);
		    }
#else
		    fprintf(stderr, "TLS not supported with your LDAP library\n");
		    exit(1);
#endif
		}
#endif
		squid_ldap_set_timelimit(ld, timelimit);
		squid_ldap_set_referrals(ld, !noreferrals);
		squid_ldap_set_aliasderef(ld, aliasderef);
		if (binddn && bindpasswd && *binddn && *bindpasswd) {
		    rc = ldap_simple_bind_s(ld, binddn, bindpasswd);
		    if (rc != LDAP_SUCCESS) {
			fprintf(stderr, PROGRAM_NAME " WARNING, could not bind to binddn '%s'\n", ldap_err2string(rc));
			ldap_unbind(ld);
			ld = NULL;
			break;
		    }
		}
		if (debug)
		    fprintf(stderr, "Connected OK\n");
	    }
	    if (searchLDAP(ld, group, user, extension_dn) == 0) {
		found = 1;
		break;
	    } else {
		if (tryagain) {
		    tryagain = 0;
		    ldap_unbind(ld);
		    ld = NULL;
		    goto recover;
		}
	    }
	}
	if (found)
	    printf("OK\n");
	else
	    printf("ERR\n");

	if (ld != NULL) {
	    if (!persistent || (squid_ldap_errno(ld) != LDAP_SUCCESS && squid_ldap_errno(ld) != LDAP_INVALID_CREDENTIALS)) {
		ldap_unbind(ld);
		ld = NULL;
	    } else {
		tryagain = 1;
	    }
	}
    }
    if (ld)
	ldap_unbind(ld);
    return 0;
}

static int
ldap_escape_value(char *escaped, int size, const char *src)
{
    int n = 0;
    while (size > 4 && *src) {
	switch(*src) {
	case '*':
	case '(':
	case ')':
	case '\\':
	    n += 3;
	    size -= 3;
	    if (size > 0) {
		*escaped++ = '\\';
		snprintf(escaped, 3, "%02x", (unsigned char)*src++);
		escaped+=2;
	    }
	    break;
	default:
	    *escaped++ = *src++;
	    n++;
	    size--;
	}
    }
    *escaped = '\0';
    return n;
}

static int
build_filter(char *filter, int size, const char *template, const char *user, const char *group)
{
    int n;
    while(*template && size > 0) {
	switch(*template) {
	case '%':
	    template++;
	    switch (*template) {
	    case 'u':
	    case 'v':
		template++;
		n = ldap_escape_value(filter, size, user);
		size -= n;
		filter += n;
		break;
	    case 'g':
	    case 'a':
		template++;
		n = ldap_escape_value(filter, size, group);
		size -= n;
		filter += n;
		break;
	    default:
		fprintf(stderr, "ERROR: Unknown filter template string %%%c\n", *template);
		return 1;
		break;
	    }
	    break;
	case '\\':
	    template++;
	    if (*template) {
		*filter++ = *template++;
		size--;
	    }
	    break;
	default:
	    *filter++ = *template++;
	    size--;
	    break;
	}
    }
    if (size <= 0) {
	fprintf(stderr, "ERROR: Filter too large\n");
	return 1;
    }
    *filter = '\0';
    return 0;
}

static int
searchLDAPGroup(LDAP * ld, char *group, char *member, char *extension_dn)
{
    char filter[256];
    static char searchbase[256];
    LDAPMessage *res = NULL;
    LDAPMessage *entry;
    int rc;
    char *searchattr[] = {LDAP_NO_ATTRS, NULL};

    if (extension_dn && *extension_dn)
	snprintf(searchbase, sizeof(searchbase), "%s,%s", extension_dn, basedn);
    else
	snprintf(searchbase, sizeof(searchbase), "%s", basedn);

    if (build_filter(filter, sizeof(filter), searchfilter, member, group) != 0) {
	fprintf(stderr, PROGRAM_NAME " ERROR, Failed to construct LDAP search filter. filter=\"%s\", user=\"%s\", group=\"%s\"\n", filter, member, group);
	return 1;
    }

    if (debug)
	fprintf(stderr, "group filter '%s', searchbase '%s'\n", filter, searchbase);

    rc = ldap_search_s(ld, searchbase, searchscope, filter, searchattr, 1, &res);
    if (rc != LDAP_SUCCESS) {
	if (noreferrals && rc == LDAP_PARTIAL_RESULTS) {
	    /* Everything is fine. This is expected when referrals
	     * are disabled.
	     */
	} else {
	    fprintf(stderr, PROGRAM_NAME " WARNING, LDAP search error '%s'\n", ldap_err2string(rc));
#if defined(NETSCAPE_SSL)
	    if (sslpath && ((rc == LDAP_SERVER_DOWN) || (rc == LDAP_CONNECT_ERROR))) {
		int sslerr = PORT_GetError();
		fprintf(stderr, PROGRAM_NAME ": WARNING, SSL error %d (%s)\n", sslerr, ldapssl_err2string(sslerr));
	    }
#endif
	    ldap_msgfree(res);
	    return 1;
	}
    }
    entry = ldap_first_entry(ld, res);
    if (!entry) {
	ldap_msgfree(res);
	return 1;
    }
    ldap_msgfree(res);
    return 0;
}

static int
searchLDAP(LDAP *ld, char *group, char *login, char *extension_dn)
{

    if (usersearchfilter) {
	char filter[8192];
	char searchbase[8192];
	char escaped_login[1024];
	LDAPMessage *res = NULL;
	LDAPMessage *entry;
	int rc;
	char *userdn;
	char *searchattr[] = {LDAP_NO_ATTRS, NULL};
	if (extension_dn && *extension_dn)
	    snprintf(searchbase, sizeof(searchbase), "%s,%s", extension_dn, userbasedn ? userbasedn : basedn);
	else
	    snprintf(searchbase, sizeof(searchbase), "%s", userbasedn ? userbasedn : basedn);
	ldap_escape_value(escaped_login, sizeof(escaped_login), login);
	snprintf(filter, sizeof(filter), usersearchfilter, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login, escaped_login);
	if (debug)
	    fprintf(stderr, "user filter '%s', searchbase '%s'\n", filter, searchbase);
	rc = ldap_search_s(ld, searchbase, searchscope, filter, searchattr, 1, &res);
	if (rc != LDAP_SUCCESS) {
	    if (noreferrals && rc == LDAP_PARTIAL_RESULTS) {
		/* Everything is fine. This is expected when referrals
		 * are disabled.
		 */
	    } else {
		fprintf(stderr, PROGRAM_NAME " WARNING, LDAP search error '%s'\n", ldap_err2string(rc));
#if defined(NETSCAPE_SSL)
		if (sslpath && ((rc == LDAP_SERVER_DOWN) || (rc == LDAP_CONNECT_ERROR))) {
		    int sslerr = PORT_GetError();
		    fprintf(stderr, PROGRAM_NAME ": WARNING, SSL error %d (%s)\n", sslerr, ldapssl_err2string(sslerr));
		}
#endif
		ldap_msgfree(res);
		return 1;
	    }
	}
	entry = ldap_first_entry(ld, res);
	if (!entry) {
	    fprintf(stderr, PROGRAM_NAME " WARNING, User '%s' not found in '%s'\n", login, searchbase);
	    ldap_msgfree(res);
	    return 1;
	}
	userdn = ldap_get_dn(ld, entry);
	rc = searchLDAPGroup(ld, group, userdn, extension_dn);
	squid_ldap_memfree(userdn);
	ldap_msgfree(res);
	return rc;
    } else if (userdnattr) {
	char dn[8192];
	if (extension_dn && *extension_dn)
	    sprintf(dn, "%s=%s, %s, %s", userdnattr, login, extension_dn, userbasedn ? userbasedn : basedn);
	else
	    sprintf(dn, "%s=%s, %s", userdnattr, login, userbasedn ? userbasedn : basedn);
	return searchLDAPGroup(ld, group, dn, extension_dn);
    } else {
	return searchLDAPGroup(ld, group, login, extension_dn);
    }
}


int readSecret(char *filename)
{
  char  buf[BUFSIZ];
  char  *e=0;
  FILE  *f;

  if(!(f=fopen(filename, "r"))) {
    fprintf(stderr, PROGRAM_NAME " ERROR: Can not read secret file %s\n", filename);
    return 1;
  }

  if( !fgets(buf, sizeof(buf)-1, f)) {
    fprintf(stderr, PROGRAM_NAME " ERROR: Secret file %s is empty\n", filename);
    fclose(f);
    return 1;
  }

  /* strip whitespaces on end */
  if((e = strrchr(buf, '\n'))) *e = 0;
  if((e = strrchr(buf, '\r'))) *e = 0;

  bindpasswd = (char *) calloc(sizeof(char), strlen(buf)+1);
  if (bindpasswd) {
    strcpy(bindpasswd, buf);
  } else {
    fprintf(stderr, PROGRAM_NAME " ERROR: can not allocate memory\n"); 
  }

  fclose(f);

  return 0;
}
