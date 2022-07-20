
/*
 * $Id: cache_cf.c,v 1.396.2.26 2005/05/06 22:33:53 wessels Exp $
 *
 * DEBUG: section 3     Configuration File Parsing
 * AUTHOR: Harvest Derived
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "squid.h"

#if SQUID_SNMP
#include "snmp.h"
#endif

static const char *const T_SECOND_STR = "second";
static const char *const T_MINUTE_STR = "minute";
static const char *const T_HOUR_STR = "hour";
static const char *const T_DAY_STR = "day";
static const char *const T_WEEK_STR = "week";
static const char *const T_FORTNIGHT_STR = "fortnight";
static const char *const T_MONTH_STR = "month";
static const char *const T_YEAR_STR = "year";
static const char *const T_DECADE_STR = "decade";

static const char *const B_BYTES_STR = "bytes";
static const char *const B_KBYTES_STR = "KB";
static const char *const B_MBYTES_STR = "MB";
static const char *const B_GBYTES_STR = "GB";

static const char *const list_sep = ", \t\n\r";

static void parse_cachedir_option_readonly(SwapDir * sd, const char *option, const char *value, int reconfiguring);
static void dump_cachedir_option_readonly(StoreEntry * e, const char *option, SwapDir * sd);
static void parse_cachedir_option_maxsize(SwapDir * sd, const char *option, const char *value, int reconfiguring);
static void dump_cachedir_option_maxsize(StoreEntry * e, const char *option, SwapDir * sd);
static struct cache_dir_option common_cachedir_options[] =
{
    {"read-only", parse_cachedir_option_readonly, dump_cachedir_option_readonly},
    {"max-size", parse_cachedir_option_maxsize, dump_cachedir_option_maxsize},
    {NULL, NULL}
};


static void update_maxobjsize(void);
static void configDoConfigure(void);
static void parse_refreshpattern(refresh_t **);
static int parseTimeUnits(const char *unit);
static void parseTimeLine(time_t * tptr, const char *units);
static void parse_ushort(u_short * var);
static void parse_string(char **);
static void default_all(void);
static void defaults_if_none(void);
static int parse_line(char *);
static void parseBytesLine(squid_off_t * bptr, const char *units);
static size_t parseBytesUnits(const char *unit);
static void free_all(void);
void requirePathnameExists(const char *name, const char *path);
static OBJH dump_config;
#ifdef HTTP_VIOLATIONS
static void dump_http_header_access(StoreEntry * entry, const char *name, header_mangler header[]);
static void parse_http_header_access(header_mangler header[]);
static void free_http_header_access(header_mangler header[]);
static void dump_http_header_replace(StoreEntry * entry, const char *name, header_mangler header[]);
static void parse_http_header_replace(header_mangler * header);
static void free_http_header_replace(header_mangler * header);
#endif
static void parse_denyinfo(acl_deny_info_list ** var);
static void dump_denyinfo(StoreEntry * entry, const char *name, acl_deny_info_list * var);
static void free_denyinfo(acl_deny_info_list ** var);
static void parse_sockaddr_in_list(sockaddr_in_list **);
static void dump_sockaddr_in_list(StoreEntry *, const char *, const sockaddr_in_list *);
static void free_sockaddr_in_list(sockaddr_in_list **);
static int check_null_sockaddr_in_list(const sockaddr_in_list *);
#if USE_SSL
static void parse_https_port_list(https_port_list **);
static void dump_https_port_list(StoreEntry *, const char *, const https_port_list *);
static void free_https_port_list(https_port_list **);
#if 0
static int check_null_https_port_list(const https_port_list *);
#endif
#endif /* USE_SSL */

void
self_destruct(void)
{
    shutting_down = 1;
    fatalf("Bungled %s line %d: %s",
	cfg_filename, config_lineno, config_input_line);
}

void
wordlistDestroy(wordlist ** list)
{
    wordlist *w = NULL;
    while ((w = *list) != NULL) {
	*list = w->next;
	safe_free(w->key);
	memFree(w, MEM_WORDLIST);
    }
    *list = NULL;
}

const char *
wordlistAdd(wordlist ** list, const char *key)
{
    while (*list)
	list = &(*list)->next;
    *list = memAllocate(MEM_WORDLIST);
    (*list)->key = xstrdup(key);
    (*list)->next = NULL;
    return (*list)->key;
}

void
wordlistJoin(wordlist ** list, wordlist ** wl)
{
    while (*list)
	list = &(*list)->next;
    *list = *wl;
    *wl = NULL;
}

void
wordlistAddWl(wordlist ** list, wordlist * wl)
{
    while (*list)
	list = &(*list)->next;
    for (; wl; wl = wl->next, list = &(*list)->next) {
	*list = memAllocate(MEM_WORDLIST);
	(*list)->key = xstrdup(wl->key);
	(*list)->next = NULL;
    }
}

void
wordlistCat(const wordlist * w, MemBuf * mb)
{
    while (NULL != w) {
	memBufPrintf(mb, "%s\n", w->key);
	w = w->next;
    }
}

wordlist *
wordlistDup(const wordlist * w)
{
    wordlist *D = NULL;
    while (NULL != w) {
	wordlistAdd(&D, w->key);
	w = w->next;
    }
    return D;
}

void
intlistDestroy(intlist ** list)
{
    intlist *w = NULL;
    intlist *n = NULL;
    for (w = *list; w; w = n) {
	n = w->next;
	memFree(w, MEM_INTLIST);
    }
    *list = NULL;
}

int
intlistFind(intlist * list, int i)
{
    intlist *w = NULL;
    for (w = list; w; w = w->next)
	if (w->i == i)
	    return 1;
    return 0;
}


/*
 * Use this #define in all the parse*() functions.  Assumes char *token is
 * defined
 */

int
GetInteger(void)
{
    char *token = strtok(NULL, w_space);
    char *end;
    int i;
    double d;
    if (token == NULL)
	self_destruct();
    i = strtol(token, &end, 0);
    d = strtod(token, NULL);
    if (d > INT_MAX || end == token)
	self_destruct();
    return i;
}

static squid_off_t
GetOffT(void)
{
    char *token = strtok(NULL, w_space);
    char *end;
    squid_off_t i;
    if (token == NULL)
	self_destruct();
    i = strto_off_t(token, &end, 0);
#if SIZEOF_SQUID_OFF_T <= 4
    {
	double d = strtod(token, NULL);
	if (d > INT_MAX)
	    end = token;
    }
#endif
    if (end == token)
	self_destruct();
    return i;
}

static void
update_maxobjsize(void)
{
    int i;
    squid_off_t ms = -1;

    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	if (Config.cacheSwap.swapDirs[i].max_objsize > ms)
	    ms = Config.cacheSwap.swapDirs[i].max_objsize;
    }
    store_maxobjsize = ms;
}

int
parseConfigFile(const char *file_name)
{
    FILE *fp = NULL;
    char *token = NULL;
    char *tmp_line;
    int err_count = 0;
    configFreeMemory();
    default_all();
    if ((fp = fopen(file_name, "r")) == NULL)
	fatalf("Unable to open configuration file: %s: %s",
	    file_name, xstrerror());
#if defined(_SQUID_CYGWIN_)
    setmode(fileno(fp), O_TEXT);
#endif
    cfg_filename = file_name;
    if ((token = strrchr(cfg_filename, '/')))
	cfg_filename = token + 1;
    memset(config_input_line, '\0', BUFSIZ);
    config_lineno = 0;
    while (fgets(config_input_line, BUFSIZ, fp)) {
	config_lineno++;
	if ((token = strchr(config_input_line, '\n')))
	    *token = '\0';
	if ((token = strchr(config_input_line, '\r')))
	    *token = '\0';
	if (config_input_line[0] == '#')
	    continue;
	if (config_input_line[0] == '\0')
	    continue;
	debug(3, 5) ("Processing: '%s'\n", config_input_line);
	tmp_line = xstrdup(config_input_line);
	if (!parse_line(tmp_line)) {
	    debug(3, 0) ("parseConfigFile: line %d unrecognized: '%s'\n",
		config_lineno,
		config_input_line);
	    err_count++;
	}
	safe_free(tmp_line);
    }
    fclose(fp);
    defaults_if_none();
    if (opt_send_signal == -1) {
	configDoConfigure();
	cachemgrRegister("config",
	    "Current Squid Configuration",
	    dump_config,
	    1, 1);
    }
    return err_count;
}

static void
configDoConfigure(void)
{
    LOCAL_ARRAY(char, buf, BUFSIZ);
    memset(&Config2, '\0', sizeof(SquidConfig2));
    /* init memory as early as possible */
    memConfigure();
    /* Sanity checks */
    if (Config.cacheSwap.swapDirs == NULL)
	fatal("No cache_dir's specified in config file");
    /* calculate Config.Swap.maxSize */
    storeDirConfigure();
    if (0 == Config.Swap.maxSize)
	/* people might want a zero-sized cache on purpose */
	(void) 0;
    else if (Config.Swap.maxSize < (Config.memMaxSize >> 10))
	debug(3, 0) ("WARNING cache_mem is larger than total disk cache space!\n");
    if (Config.Announce.period > 0) {
	Config.onoff.announce = 1;
    } else if (Config.Announce.period < 1) {
	Config.Announce.period = 86400 * 365;	/* one year */
	Config.onoff.announce = 0;
    }
#if USE_DNSSERVERS
    if (Config.dnsChildren < 1)
	fatal("No dnsservers allocated");
#endif
    if (Config.Program.redirect) {
	if (Config.redirectChildren < 1) {
	    Config.redirectChildren = 0;
	    wordlistDestroy(&Config.Program.redirect);
	}
    }
    if (Config.Accel.host) {
	snprintf(buf, BUFSIZ, "http://%s:%d", Config.Accel.host, Config.Accel.port);
	Config2.Accel.prefix = xstrdup(buf);
	Config2.Accel.on = 1;
    }
    if (Config.appendDomain)
	if (*Config.appendDomain != '.')
	    fatal("append_domain must begin with a '.'");
    if (Config.errHtmlText == NULL)
	Config.errHtmlText = xstrdup(null_string);
    storeConfigure();
    if (Config2.Accel.on && !strcmp(Config.Accel.host, "virtual")) {
	vhost_mode = 1;
	if (Config.Accel.port == 0)
	    vport_mode = 1;
    }
    if (Config.Sockaddr.http == NULL)
	fatal("No http_port specified!");
    snprintf(ThisCache, sizeof(ThisCache), "%s:%d (%s)",
	uniqueHostname(),
	(int) ntohs(Config.Sockaddr.http->s.sin_port),
	full_appname_string);
    /*
     * the extra space is for loop detection in client_side.c -- we search
     * for substrings in the Via header.
     */
    snprintf(ThisCache2, sizeof(ThisCache), " %s:%d (%s)",
	uniqueHostname(),
	(int) ntohs(Config.Sockaddr.http->s.sin_port),
	full_appname_string);
    if (!Config.udpMaxHitObjsz || Config.udpMaxHitObjsz > SQUID_UDP_SO_SNDBUF)
	Config.udpMaxHitObjsz = SQUID_UDP_SO_SNDBUF;
    if (Config.appendDomain)
	Config.appendDomainLen = strlen(Config.appendDomain);
    else
	Config.appendDomainLen = 0;
    safe_free(debug_options)
	debug_options = xstrdup(Config.debugOptions);
    if (Config.retry.maxtries > 10)
	fatal("maximum_single_addr_tries cannot be larger than 10");
    if (Config.retry.maxtries < 1) {
	debug(3, 0) ("WARNING: resetting 'maximum_single_addr_tries to 1\n");
	Config.retry.maxtries = 1;
    }
    requirePathnameExists("MIME Config Table", Config.mimeTablePathname);
#if USE_DNSSERVERS
    requirePathnameExists("cache_dns_program", Config.Program.dnsserver);
#endif
#if USE_UNLINKD
    requirePathnameExists("unlinkd_program", Config.Program.unlinkd);
#endif
    if (Config.Program.redirect)
	requirePathnameExists("redirect_program", Config.Program.redirect->key);
    requirePathnameExists("Icon Directory", Config.icons.directory);
    requirePathnameExists("Error Directory", Config.errorDirectory);
#if HTTP_VIOLATIONS
    {
	const refresh_t *R;
	for (R = Config.Refresh; R; R = R->next) {
	    if (!R->flags.override_expire)
		continue;
	    debug(22, 1) ("WARNING: use of 'override-expire' in 'refresh_pattern' violates HTTP\n");
	    break;
	}
	for (R = Config.Refresh; R; R = R->next) {
	    if (!R->flags.override_lastmod)
		continue;
	    debug(22, 1) ("WARNING: use of 'override-lastmod' in 'refresh_pattern' violates HTTP\n");
	    break;
	}
    }
#endif
    if (Config.Wais.relayHost) {
	if (Config.Wais.peer)
	    cbdataFree(Config.Wais.peer);
	Config.Wais.peer = cbdataAlloc(peer);
	Config.Wais.peer->host = xstrdup(Config.Wais.relayHost);
	Config.Wais.peer->http_port = Config.Wais.relayPort;
    }
    if (aclPurgeMethodInUse(Config.accessList.http))
	Config2.onoff.enable_purge = 1;
    if (geteuid() == 0) {
	if (NULL != Config.effectiveUser) {
	    struct passwd *pwd = getpwnam(Config.effectiveUser);
	    if (NULL == pwd)
		/*
		 * Andres Kroonmaa <andre@online.ee>:
		 * Some getpwnam() implementations (Solaris?) require
		 * an available FD < 256 for opening a FILE* to the
		 * passwd file.
		 * DW:
		 * This should be safe at startup, but might still fail
		 * during reconfigure.
		 */
		fatalf("getpwnam failed to find userid for effective user '%s'",
		    Config.effectiveUser);
	    Config2.effectiveUserID = pwd->pw_uid;
	    Config2.effectiveGroupID = pwd->pw_gid;
	}
    } else {
	Config2.effectiveUserID = geteuid();
	Config2.effectiveGroupID = getegid();
    }
    if (NULL != Config.effectiveGroup) {
	struct group *grp = getgrnam(Config.effectiveGroup);
	if (NULL == grp)
	    fatalf("getgrnam failed to find groupid for effective group '%s'",
		Config.effectiveGroup);
	Config2.effectiveGroupID = grp->gr_gid;
    }
    urlExtMethodConfigure();
    if (0 == Config.onoff.client_db) {
	acl *a;
	for (a = Config.aclList; a; a = a->next) {
	    if (ACL_MAXCONN != a->type)
		continue;
	    debug(22, 0) ("WARNING: 'maxconn' ACL (%s) won't work with client_db disabled\n", a->name);
	}
    }
    if (Config.negativeDnsTtl <= 0) {
	debug(22, 0) ("WARNING: resetting negative_dns_ttl to 1 second\n");
	Config.negativeDnsTtl = 1;
    }
    if (Config.positiveDnsTtl < Config.negativeDnsTtl) {
	debug(22, 0) ("NOTICE: positive_dns_ttl must be larger than negative_dns_ttl. Resetting negative_dns_ttl to match\n");
	Config.positiveDnsTtl = Config.negativeDnsTtl;
    }
#if SIZEOF_SQUID_FILE_SZ <= 4
#if SIZEOF_SQUID_OFF_T <= 4
    if (Config.Store.maxObjectSize > 0x7FFF0000) {
	debug(22, 0) ("NOTICE: maximum_object_size limited to %d KB due to hardware limitations\n", 0x7FFF0000 / 1024);
	Config.Store.maxObjectSize = 0x7FFF0000;
    }
#elif SIZEOF_OFF_T <= 4
    if (Config.Store.maxObjectSize > 0xFFFF0000) {
	debug(22, 0) ("NOTICE: maximum_object_size limited to %d KB due to OS limitations\n", 0xFFFF0000 / 1024);
	Config.Store.maxObjectSize = 0xFFFF0000;
    }
#else
    if (Config.Store.maxObjectSize > 0xFFFF0000) {
	debug(22, 0) ("NOTICE: maximum_object_size limited to %d KB to keep compatibility with existing cache\n", 0xFFFF0000 / 1024);
	Config.Store.maxObjectSize = 0xFFFF0000;
    }
#endif
#endif
    if (Config.Store.maxInMemObjSize > 8 * 1024 * 1024)
	debug(22, 0) ("WARNING: Very large maximum_object_size_in_memory settings can have negative impact on performance\n");
}

/* Parse a time specification from the config file.  Store the
 * result in 'tptr', after converting it to 'units' */
static void
parseTimeLine(time_t * tptr, const char *units)
{
    char *token;
    double d;
    time_t m;
    time_t u;
    if ((u = parseTimeUnits(units)) == 0)
	self_destruct();
    if ((token = strtok(NULL, w_space)) == NULL)
	self_destruct();
    d = atof(token);
    m = u;			/* default to 'units' if none specified */
    if (0 == d)
	(void) 0;
    else if ((token = strtok(NULL, w_space)) == NULL)
	debug(3, 0) ("WARNING: No units on '%s', assuming %f %s\n",
	    config_input_line, d, units);
    else if ((m = parseTimeUnits(token)) == 0)
	self_destruct();
    *tptr = m * d / u;
}

static int
parseTimeUnits(const char *unit)
{
    if (!strncasecmp(unit, T_SECOND_STR, strlen(T_SECOND_STR)))
	return 1;
    if (!strncasecmp(unit, T_MINUTE_STR, strlen(T_MINUTE_STR)))
	return 60;
    if (!strncasecmp(unit, T_HOUR_STR, strlen(T_HOUR_STR)))
	return 3600;
    if (!strncasecmp(unit, T_DAY_STR, strlen(T_DAY_STR)))
	return 86400;
    if (!strncasecmp(unit, T_WEEK_STR, strlen(T_WEEK_STR)))
	return 86400 * 7;
    if (!strncasecmp(unit, T_FORTNIGHT_STR, strlen(T_FORTNIGHT_STR)))
	return 86400 * 14;
    if (!strncasecmp(unit, T_MONTH_STR, strlen(T_MONTH_STR)))
	return 86400 * 30;
    if (!strncasecmp(unit, T_YEAR_STR, strlen(T_YEAR_STR)))
	return 86400 * 365.2522;
    if (!strncasecmp(unit, T_DECADE_STR, strlen(T_DECADE_STR)))
	return 86400 * 365.2522 * 10;
    debug(3, 1) ("parseTimeUnits: unknown time unit '%s'\n", unit);
    return 0;
}

static void
parseBytesLine(squid_off_t * bptr, const char *units)
{
    char *token;
    double d;
    squid_off_t m;
    squid_off_t u;
    if ((u = parseBytesUnits(units)) == 0)
	self_destruct();
    if ((token = strtok(NULL, w_space)) == NULL)
	self_destruct();
    if (strcmp(token, "none") == 0 || strcmp(token, "-1") == 0) {
	*bptr = (squid_off_t) - 1;
	return;
    }
    d = atof(token);
    m = u;			/* default to 'units' if none specified */
    if (0.0 == d)
	(void) 0;
    else if ((token = strtok(NULL, w_space)) == NULL)
	debug(3, 0) ("WARNING: No units on '%s', assuming %f %s\n",
	    config_input_line, d, units);
    else if ((m = parseBytesUnits(token)) == 0)
	self_destruct();
    *bptr = m * d / u;
    if ((double) *bptr * 2 != m * d / u * 2)
	self_destruct();
}

static size_t
parseBytesUnits(const char *unit)
{
    if (!strncasecmp(unit, B_BYTES_STR, strlen(B_BYTES_STR)))
	return 1;
    if (!strncasecmp(unit, B_KBYTES_STR, strlen(B_KBYTES_STR)))
	return 1 << 10;
    if (!strncasecmp(unit, B_MBYTES_STR, strlen(B_MBYTES_STR)))
	return 1 << 20;
    if (!strncasecmp(unit, B_GBYTES_STR, strlen(B_GBYTES_STR)))
	return 1 << 30;
    debug(3, 1) ("parseBytesUnits: unknown bytes unit '%s'\n", unit);
    return 0;
}

/*****************************************************************************
 * Max
 *****************************************************************************/

static void
dump_acl(StoreEntry * entry, const char *name, acl * ae)
{
    wordlist *w;
    wordlist *v;
    while (ae != NULL) {
	debug(3, 3) ("dump_acl: %s %s\n", name, ae->name);
	v = w = aclDumpGeneric(ae);
	while (v != NULL) {
	    debug(3, 3) ("dump_acl: %s %s %s\n", name, ae->name, v->key);
	    storeAppendPrintf(entry, "%s %s %s %s\n",
		name,
		ae->name,
		aclTypeToStr(ae->type),
		v->key);
	    v = v->next;
	}
	wordlistDestroy(&w);
	ae = ae->next;
    }
}

static void
parse_acl(acl ** ae)
{
    aclParseAclLine(ae);
}

static void
free_acl(acl ** ae)
{
    aclDestroyAcls(ae);
}

static void
dump_acl_list(StoreEntry * entry, acl_list * head)
{
    acl_list *l;
    for (l = head; l; l = l->next) {
	storeAppendPrintf(entry, " %s%s",
	    l->op ? null_string : "!",
	    l->acl->name);
    }
}

static void
dump_acl_access(StoreEntry * entry, const char *name, acl_access * head)
{
    acl_access *l;
    for (l = head; l; l = l->next) {
	storeAppendPrintf(entry, "%s %s",
	    name,
	    l->allow ? "Allow" : "Deny");
	dump_acl_list(entry, l->acl_list);
	storeAppendPrintf(entry, "\n");
    }
}

static void
parse_acl_access(acl_access ** head)
{
    aclParseAccessLine(head);
}

static void
free_acl_access(acl_access ** head)
{
    aclDestroyAccessList(head);
}

static void
dump_address(StoreEntry * entry, const char *name, struct in_addr addr)
{
    storeAppendPrintf(entry, "%s %s\n", name, inet_ntoa(addr));
}

static void
parse_address(struct in_addr *addr)
{
    const struct hostent *hp;
    char *token = strtok(NULL, w_space);

    if (token == NULL)
	self_destruct();
    if (safe_inet_addr(token, addr) == 1)
	(void) 0;
    else if ((hp = gethostbyname(token)))	/* dont use ipcache */
	*addr = inaddrFromHostent(hp);
    else
	self_destruct();
}

static void
free_address(struct in_addr *addr)
{
    memset(addr, '\0', sizeof(struct in_addr));
}

CBDATA_TYPE(acl_address);

static void
dump_acl_address(StoreEntry * entry, const char *name, acl_address * head)
{
    acl_address *l;
    for (l = head; l; l = l->next) {
	if (l->addr.s_addr != INADDR_ANY)
	    storeAppendPrintf(entry, "%s %s", name, inet_ntoa(l->addr));
	else
	    storeAppendPrintf(entry, "%s autoselect", name);
	dump_acl_list(entry, l->acl_list);
	storeAppendPrintf(entry, "\n");
    }
}

static void
freed_acl_address(void *data)
{
    acl_address *l = data;
    aclDestroyAclList(&l->acl_list);
}

static void
parse_acl_address(acl_address ** head)
{
    acl_address *l;
    acl_address **tail = head;	/* sane name below */
    CBDATA_INIT_TYPE_FREECB(acl_address, freed_acl_address);
    l = cbdataAlloc(acl_address);
    parse_address(&l->addr);
    aclParseAclList(&l->acl_list);
    while (*tail)
	tail = &(*tail)->next;
    *tail = l;
}

static void
free_acl_address(acl_address ** head)
{
    while (*head) {
	acl_address *l = *head;
	*head = l->next;
	cbdataFree(l);
    }
}

CBDATA_TYPE(acl_tos);

static void
dump_acl_tos(StoreEntry * entry, const char *name, acl_tos * head)
{
    acl_tos *l;
    for (l = head; l; l = l->next) {
	if (l->tos > 0)
	    storeAppendPrintf(entry, "%s 0x%02X", name, l->tos);
	else
	    storeAppendPrintf(entry, "%s none", name);
	dump_acl_list(entry, l->acl_list);
	storeAppendPrintf(entry, "\n");
    }
}

static void
freed_acl_tos(void *data)
{
    acl_tos *l = data;
    aclDestroyAclList(&l->acl_list);
}

static void
parse_acl_tos(acl_tos ** head)
{
    acl_tos *l;
    acl_tos **tail = head;	/* sane name below */
    int tos;
    char junk;
    char *token = strtok(NULL, w_space);
    if (!token)
	self_destruct();
    if (sscanf(token, "0x%x%c", &tos, &junk) != 1)
	self_destruct();
    if (tos < 0 || tos > 255)
	self_destruct();
    CBDATA_INIT_TYPE_FREECB(acl_tos, freed_acl_tos);
    l = cbdataAlloc(acl_tos);
    l->tos = tos;
    aclParseAclList(&l->acl_list);
    while (*tail)
	tail = &(*tail)->next;
    *tail = l;
}

static void
free_acl_tos(acl_tos ** head)
{
    while (*head) {
	acl_tos *l = *head;
	*head = l->next;
	l->next = NULL;
	cbdataFree(l);
    }
}

#if DELAY_POOLS

/* do nothing - free_delay_pool_count is the magic free function.
 * this is why delay_pool_count isn't just marked TYPE: ushort
 */
#define free_delay_pool_class(X)
#define free_delay_pool_access(X)
#define free_delay_pool_rates(X)
#define dump_delay_pool_class(X, Y, Z)
#define dump_delay_pool_access(X, Y, Z)
#define dump_delay_pool_rates(X, Y, Z)

static void
free_delay_pool_count(delayConfig * cfg)
{
    int i;

    if (!cfg->pools)
	return;
    for (i = 0; i < cfg->pools; i++) {
	if (cfg->class[i]) {
	    delayFreeDelayPool(i);
	    safe_free(cfg->rates[i]);
	}
	aclDestroyAccessList(&cfg->access[i]);
    }
    delayFreeDelayData(cfg->pools);
    xfree(cfg->class);
    xfree(cfg->rates);
    xfree(cfg->access);
    memset(cfg, 0, sizeof(*cfg));
}

static void
dump_delay_pool_count(StoreEntry * entry, const char *name, delayConfig cfg)
{
    int i;
    LOCAL_ARRAY(char, nom, 32);

    if (!cfg.pools) {
	storeAppendPrintf(entry, "%s 0\n", name);
	return;
    }
    storeAppendPrintf(entry, "%s %d\n", name, cfg.pools);
    for (i = 0; i < cfg.pools; i++) {
	storeAppendPrintf(entry, "delay_class %d %d\n", i + 1, cfg.class[i]);
	snprintf(nom, 32, "delay_access %d", i + 1);
	dump_acl_access(entry, nom, cfg.access[i]);
	if (cfg.class[i] >= 1)
	    storeAppendPrintf(entry, "delay_parameters %d %d/%d", i + 1,
		cfg.rates[i]->aggregate.restore_bps,
		cfg.rates[i]->aggregate.max_bytes);
	if (cfg.class[i] >= 3)
	    storeAppendPrintf(entry, " %d/%d",
		cfg.rates[i]->network.restore_bps,
		cfg.rates[i]->network.max_bytes);
	if (cfg.class[i] >= 2)
	    storeAppendPrintf(entry, " %d/%d",
		cfg.rates[i]->individual.restore_bps,
		cfg.rates[i]->individual.max_bytes);
	if (cfg.class[i] >= 1)
	    storeAppendPrintf(entry, "\n");
    }
}

static void
parse_delay_pool_count(delayConfig * cfg)
{
    if (cfg->pools) {
	debug(3, 0) ("parse_delay_pool_count: multiple delay_pools lines, aborting all previous delay_pools config\n");
	free_delay_pool_count(cfg);
    }
    parse_ushort(&cfg->pools);
    if (cfg->pools) {
	delayInitDelayData(cfg->pools);
	cfg->class = xcalloc(cfg->pools, sizeof(u_char));
	cfg->rates = xcalloc(cfg->pools, sizeof(delaySpecSet *));
	cfg->access = xcalloc(cfg->pools, sizeof(acl_access *));
    }
}

static void
parse_delay_pool_class(delayConfig * cfg)
{
    ushort pool, class;

    parse_ushort(&pool);
    if (pool < 1 || pool > cfg->pools) {
	debug(3, 0) ("parse_delay_pool_class: Ignoring pool %d not in 1 .. %d\n", pool, cfg->pools);
	return;
    }
    parse_ushort(&class);
    if (class < 1 || class > 3) {
	debug(3, 0) ("parse_delay_pool_class: Ignoring pool %d class %d not in 1 .. 3\n", pool, class);
	return;
    }
    pool--;
    if (cfg->class[pool]) {
	delayFreeDelayPool(pool);
	safe_free(cfg->rates[pool]);
    }
    /* Allocates a "delaySpecSet" just as large as needed for the class */
    cfg->rates[pool] = xmalloc(class * sizeof(delaySpec));
    cfg->class[pool] = class;
    cfg->rates[pool]->aggregate.restore_bps = cfg->rates[pool]->aggregate.max_bytes = -1;
    if (cfg->class[pool] >= 3)
	cfg->rates[pool]->network.restore_bps = cfg->rates[pool]->network.max_bytes = -1;
    if (cfg->class[pool] >= 2)
	cfg->rates[pool]->individual.restore_bps = cfg->rates[pool]->individual.max_bytes = -1;
    delayCreateDelayPool(pool, class);
}

static void
parse_delay_pool_rates(delayConfig * cfg)
{
    ushort pool, class;
    int i;
    delaySpec *ptr;
    char *token;

    parse_ushort(&pool);
    if (pool < 1 || pool > cfg->pools) {
	debug(3, 0) ("parse_delay_pool_rates: Ignoring pool %d not in 1 .. %d\n", pool, cfg->pools);
	return;
    }
    pool--;
    class = cfg->class[pool];
    if (class == 0) {
	debug(3, 0) ("parse_delay_pool_rates: Ignoring pool %d attempt to set rates with class not set\n", pool + 1);
	return;
    }
    ptr = (delaySpec *) cfg->rates[pool];
    /* read in "class" sets of restore,max pairs */
    while (class--) {
	token = strtok(NULL, "/");
	if (token == NULL)
	    self_destruct();
	if (sscanf(token, "%d", &i) != 1)
	    self_destruct();
	ptr->restore_bps = i;
	i = GetInteger();
	ptr->max_bytes = i;
	ptr++;
    }
    class = cfg->class[pool];
    /* if class is 3, swap around network and individual */
    if (class == 3) {
	delaySpec tmp;

	tmp = cfg->rates[pool]->individual;
	cfg->rates[pool]->individual = cfg->rates[pool]->network;
	cfg->rates[pool]->network = tmp;
    }
    /* initialize the delay pools */
    delayInitDelayPool(pool, class, cfg->rates[pool]);
}

static void
parse_delay_pool_access(delayConfig * cfg)
{
    ushort pool;

    parse_ushort(&pool);
    if (pool < 1 || pool > cfg->pools) {
	debug(3, 0) ("parse_delay_pool_rates: Ignoring pool %d not in 1 .. %d\n", pool, cfg->pools);
	return;
    }
    aclParseAccessLine(&cfg->access[pool - 1]);
}
#endif

#ifdef HTTP_VIOLATIONS
static void
dump_http_header_access(StoreEntry * entry, const char *name, header_mangler header[])
{
    int i;
    header_mangler *other;
    for (i = 0; i < HDR_ENUM_END; i++) {
	if (header[i].access_list == NULL)
	    continue;
	storeAppendPrintf(entry, "%s ", name);
	dump_acl_access(entry, httpHeaderNameById(i),
	    header[i].access_list);
    }
    for (other = header[HDR_OTHER].next; other; other = other->next) {
	if (other->access_list == NULL)
	    continue;
	storeAppendPrintf(entry, "%s ", name);
	dump_acl_access(entry, other->name,
	    other->access_list);
    }
}

static void
parse_http_header_access(header_mangler header[])
{
    int id, i;
    char *t = NULL;
    if ((t = strtok(NULL, w_space)) == NULL) {
	debug(3, 0) ("%s line %d: %s\n",
	    cfg_filename, config_lineno, config_input_line);
	debug(3, 0) ("parse_http_header_access: missing header name.\n");
	return;
    }
    /* Now lookup index of header. */
    id = httpHeaderIdByNameDef(t, strlen(t));
    if (strcmp(t, "All") == 0)
	id = HDR_ENUM_END;
    else if (strcmp(t, "Other") == 0)
	id = HDR_OTHER;
    else if (id == -1) {
	header_mangler *hdr = header[HDR_OTHER].next;
	while (hdr && strcasecmp(hdr->name, t) != 0)
	    hdr = hdr->next;
	if (!hdr) {
	    hdr = xcalloc(1, sizeof *hdr);
	    hdr->name = xstrdup(t);
	    hdr->next = header[HDR_OTHER].next;
	    header[HDR_OTHER].next = hdr;
	}
	parse_acl_access(&hdr->access_list);
	return;
    }
    if (id != HDR_ENUM_END) {
	parse_acl_access(&header[id].access_list);
    } else {
	char *next_string = t + strlen(t) - 1;
	*next_string = 'A';
	*(next_string + 1) = ' ';
	for (i = 0; i < HDR_ENUM_END; i++) {
	    char *new_string = xstrdup(next_string);
	    strtok(new_string, w_space);
	    parse_acl_access(&header[i].access_list);
	    safe_free(new_string);
	}
    }
}

static void
free_http_header_access(header_mangler header[])
{
    int i;
    header_mangler **hdrp;
    for (i = 0; i < HDR_ENUM_END; i++) {
	free_acl_access(&header[i].access_list);
    }
    hdrp = &header[HDR_OTHER].next;
    while (*hdrp) {
	header_mangler *hdr = *hdrp;
	free_acl_access(&hdr->access_list);
	if (!hdr->replacement) {
	    *hdrp = hdr->next;
	    safe_free(hdr->name);
	    safe_free(hdr);
	} else {
	    hdrp = &hdr->next;
	}
    }
}

static void
dump_http_header_replace(StoreEntry * entry, const char *name, header_mangler
    header[])
{
    int i;
    header_mangler *other;
    for (i = 0; i < HDR_ENUM_END; i++) {
	if (NULL == header[i].replacement)
	    continue;
	storeAppendPrintf(entry, "%s %s %s\n", name, httpHeaderNameById(i),
	    header[i].replacement);
    }
    for (other = header[HDR_OTHER].next; other; other = other->next) {
	if (other->replacement == NULL)
	    continue;
	storeAppendPrintf(entry, "%s %s %s\n", name, other->name, other->replacement);
    }
}

static void
parse_http_header_replace(header_mangler header[])
{
    int id, i;
    char *t = NULL;
    if ((t = strtok(NULL, w_space)) == NULL) {
	debug(3, 0) ("%s line %d: %s\n",
	    cfg_filename, config_lineno, config_input_line);
	debug(3, 0) ("parse_http_header_replace: missing header name.\n");
	return;
    }
    /* Now lookup index of header. */
    id = httpHeaderIdByNameDef(t, strlen(t));
    if (strcmp(t, "All") == 0)
	id = HDR_ENUM_END;
    else if (strcmp(t, "Other") == 0)
	id = HDR_OTHER;
    else if (id == -1) {
	header_mangler *hdr = header[HDR_OTHER].next;
	while (hdr && strcasecmp(hdr->name, t) != 0)
	    hdr = hdr->next;
	if (!hdr) {
	    hdr = xcalloc(1, sizeof *hdr);
	    hdr->name = xstrdup(t);
	    hdr->next = header[HDR_OTHER].next;
	    header[HDR_OTHER].next = hdr;
	}
	if (hdr->replacement != NULL)
	    safe_free(hdr->replacement);
	hdr->replacement = xstrdup(t + strlen(t) + 1);
	return;
    }
    if (id != HDR_ENUM_END) {
	if (header[id].replacement != NULL)
	    safe_free(header[id].replacement);
	header[id].replacement = xstrdup(t + strlen(t) + 1);
    } else {
	for (i = 0; i < HDR_ENUM_END; i++) {
	    if (header[i].replacement != NULL)
		safe_free(header[i].replacement);
	    header[i].replacement = xstrdup(t + strlen(t) + 1);
	}
    }
}

static void
free_http_header_replace(header_mangler header[])
{
    int i;
    header_mangler **hdrp;
    for (i = 0; i < HDR_ENUM_END; i++) {
	if (header[i].replacement != NULL)
	    safe_free(header[i].replacement);
    }
    hdrp = &header[HDR_OTHER].next;
    while (*hdrp) {
	header_mangler *hdr = *hdrp;
	free_acl_access(&hdr->access_list);
	if (!hdr->access_list) {
	    *hdrp = hdr->next;
	    safe_free(hdr->name);
	    safe_free(hdr);
	} else {
	    hdrp = &hdr->next;
	}
    }
}
#endif

void
dump_cachedir_options(StoreEntry * entry, struct cache_dir_option *options, SwapDir * sd)
{
    struct cache_dir_option *option;
    if (!options)
	return;
    for (option = options; option->name; option++)
	option->dump(entry, option->name, sd);
}

static void
dump_cachedir(StoreEntry * entry, const char *name, cacheSwap swap)
{
    SwapDir *s;
    int i;
    for (i = 0; i < swap.n_configured; i++) {
	s = swap.swapDirs + i;
	storeAppendPrintf(entry, "%s %s %s", name, s->type, s->path);
	if (s->dump)
	    s->dump(entry, s);
	dump_cachedir_options(entry, common_cachedir_options, s);
	storeAppendPrintf(entry, "\n");
    }
}

static int
check_null_cachedir(cacheSwap swap)
{
    return swap.swapDirs == NULL;
}

static int
check_null_string(char *s)
{
    return s == NULL;
}

static void
allocate_new_authScheme(authConfig * cfg)
{
    if (cfg->schemes == NULL) {
	cfg->n_allocated = 4;
	cfg->schemes = xcalloc(cfg->n_allocated, sizeof(authScheme));
    }
    if (cfg->n_allocated == cfg->n_configured) {
	authScheme *tmp;
	cfg->n_allocated <<= 1;
	tmp = xcalloc(cfg->n_allocated, sizeof(authScheme));
	xmemcpy(tmp, cfg->schemes, cfg->n_configured * sizeof(authScheme));
	xfree(cfg->schemes);
	cfg->schemes = tmp;
    }
}

static void
parse_authparam(authConfig * config)
{
    char *type_str;
    char *param_str;
    authScheme *scheme = NULL;
    int type, i;

    if ((type_str = strtok(NULL, w_space)) == NULL)
	self_destruct();

    if ((param_str = strtok(NULL, w_space)) == NULL)
	self_destruct();

    if ((type = authenticateAuthSchemeId(type_str)) == -1) {
	debug(3, 0) ("Parsing Config File: Unknown authentication scheme '%s'.\n", type_str);
	return;
    }
    for (i = 0; i < config->n_configured; i++) {
	if (config->schemes[i].Id == type) {
	    scheme = config->schemes + i;
	}
    }

    if (scheme == NULL) {
	allocate_new_authScheme(config);
	scheme = config->schemes + config->n_configured;
	config->n_configured++;
	scheme->Id = type;
	scheme->typestr = authscheme_list[type].typestr;
    }
    authscheme_list[type].parse(scheme, config->n_configured, param_str);
}

static void
free_authparam(authConfig * cfg)
{
    authScheme *scheme;
    int i;
    /* DON'T FREE THESE FOR RECONFIGURE */
    if (reconfiguring)
	return;
    for (i = 0; i < cfg->n_configured; i++) {
	scheme = cfg->schemes + i;
	authscheme_list[scheme->Id].freeconfig(scheme);
    }
    safe_free(cfg->schemes);
    cfg->schemes = NULL;
    cfg->n_allocated = 0;
    cfg->n_configured = 0;
}

static void
dump_authparam(StoreEntry * entry, const char *name, authConfig cfg)
{
    authScheme *scheme;
    int i;
    for (i = 0; i < cfg.n_configured; i++) {
	scheme = cfg.schemes + i;
	authscheme_list[scheme->Id].dump(entry, name, scheme);
    }
}

void
allocate_new_swapdir(cacheSwap * swap)
{
    if (swap->swapDirs == NULL) {
	swap->n_allocated = 4;
	swap->swapDirs = xcalloc(swap->n_allocated, sizeof(SwapDir));
    }
    if (swap->n_allocated == swap->n_configured) {
	SwapDir *tmp;
	swap->n_allocated <<= 1;
	tmp = xcalloc(swap->n_allocated, sizeof(SwapDir));
	xmemcpy(tmp, swap->swapDirs, swap->n_configured * sizeof(SwapDir));
	xfree(swap->swapDirs);
	swap->swapDirs = tmp;
    }
}

static int
find_fstype(char *type)
{
    int i;
    for (i = 0; storefs_list[i].typestr != NULL; i++) {
	if (strcasecmp(type, storefs_list[i].typestr) == 0) {
	    return i;
	}
    }
    return (-1);
}

static void
parse_cachedir(cacheSwap * swap)
{
    char *type_str;
    char *path_str;
    SwapDir *sd;
    int i;
    int fs;

    if ((type_str = strtok(NULL, w_space)) == NULL)
	self_destruct();

    if ((path_str = strtok(NULL, w_space)) == NULL)
	self_destruct();

    /*
     * This bit of code is a little strange.
     * See, if we find a path and type match for a given line, then
     * as long as we're reconfiguring, we can just call its reconfigure
     * function. No harm there.
     *
     * Trouble is, if we find a path match, but not a type match, we have
     * a dilemma - we could gracefully shut down the fs, kill it, and
     * create a new one of a new type in its place, BUT at this stage the
     * fs is meant to be the *NEW* one, and so things go very strange. :-)
     *
     * So, we'll assume the person isn't going to change the fs type for now,
     * and XXX later on we will make sure that its picked up.
     *
     * (moving around cache_dir lines will be looked at later in a little
     * more sane detail..)
     */

    for (i = 0; i < swap->n_configured; i++) {
	if (0 == strcasecmp(path_str, swap->swapDirs[i].path)) {
	    /* This is a little weird, you'll appreciate it later */
	    fs = find_fstype(type_str);
	    if (fs < 0) {
		fatalf("Unknown cache_dir type '%s'\n", type_str);
	    }
	    sd = swap->swapDirs + i;
	    storefs_list[fs].reconfigurefunc(sd, i, path_str);
	    update_maxobjsize();
	    return;
	}
    }

    assert(swap->n_configured < 63);	/* 7 bits, signed */

    fs = find_fstype(type_str);
    if (fs < 0) {
	/* If we get here, we didn't find a matching cache_dir type */
	fatalf("Unknown cache_dir type '%s'\n", type_str);
    }
    allocate_new_swapdir(swap);
    sd = swap->swapDirs + swap->n_configured;
    sd->type = storefs_list[fs].typestr;
    /* defaults in case fs implementation fails to set these */
    sd->max_objsize = -1;
    sd->fs.blksize = 1024;
    /* parse the FS parameters and options */
    storefs_list[fs].parsefunc(sd, swap->n_configured, path_str);
    swap->n_configured++;
    /* Update the max object size */
    update_maxobjsize();
}

static void
parse_cachedir_option_readonly(SwapDir * sd, const char *option, const char *value, int reconfiguring)
{
    int read_only = 0;
    if (value)
	read_only = atoi(value);
    else
	read_only = 1;
    sd->flags.read_only = read_only;
}

static void
dump_cachedir_option_readonly(StoreEntry * e, const char *option, SwapDir * sd)
{
    if (sd->flags.read_only)
	storeAppendPrintf(e, " %s", option);
}

static void
parse_cachedir_option_maxsize(SwapDir * sd, const char *option, const char *value, int reconfiguring)
{
    squid_off_t size;

    if (!value)
	self_destruct();

    size = strto_off_t(value, NULL, 10);

    if (reconfiguring && sd->max_objsize != size)
	debug(3, 1) ("Cache dir '%s' max object size now %ld\n", sd->path, (long int) size);

    sd->max_objsize = size;
}

static void
dump_cachedir_option_maxsize(StoreEntry * e, const char *option, SwapDir * sd)
{
    if (sd->max_objsize != -1)
	storeAppendPrintf(e, " %s=%ld", option, (long int) sd->max_objsize);
}

void
parse_cachedir_options(SwapDir * sd, struct cache_dir_option *options, int reconfiguring)
{
    int old_read_only = sd->flags.read_only;
    char *name, *value;
    struct cache_dir_option *option, *op;

    while ((name = strtok(NULL, w_space)) != NULL) {
	value = strchr(name, '=');
	if (value)
	    *value++ = '\0';	/* cut on = */
	option = NULL;
	if (options) {
	    for (op = options; !option && op->name; op++) {
		if (strcmp(op->name, name) == 0) {
		    option = op;
		    break;
		}
	    }
	}
	for (op = common_cachedir_options; !option && op->name; op++) {
	    if (strcmp(op->name, name) == 0) {
		option = op;
		break;
	    }
	}
	if (!option || !option->parse)
	    self_destruct();
	option->parse(sd, name, value, reconfiguring);
    }
    /*
     * Handle notifications about reconfigured single-options with no value
     * where the removal of the option cannot be easily detected in the
     * parsing...
     */
    if (reconfiguring) {
	if (old_read_only != sd->flags.read_only) {
	    debug(3, 1) ("Cache dir '%s' now %s\n",
		sd->path, sd->flags.read_only ? "Read-Only" : "Read-Write");
	}
    }
}

static void
free_cachedir(cacheSwap * swap)
{
    SwapDir *s;
    int i;
    /* DON'T FREE THESE FOR RECONFIGURE */
    if (reconfiguring)
	return;
    for (i = 0; i < swap->n_configured; i++) {
	s = swap->swapDirs + i;
	s->freefs(s);
	xfree(s->path);
    }
    safe_free(swap->swapDirs);
    swap->swapDirs = NULL;
    swap->n_allocated = 0;
    swap->n_configured = 0;
}

static const char *
peer_type_str(const peer_t type)
{
    switch (type) {
    case PEER_PARENT:
	return "parent";
	break;
    case PEER_SIBLING:
	return "sibling";
	break;
    case PEER_MULTICAST:
	return "multicast";
	break;
    default:
	return "unknown";
	break;
    }
}

static void
dump_peer(StoreEntry * entry, const char *name, peer * p)
{
    domain_ping *d;
    domain_type *t;
    LOCAL_ARRAY(char, xname, 128);
    while (p != NULL) {
	storeAppendPrintf(entry, "%s %s %s %d %d",
	    name,
	    p->host,
	    neighborTypeStr(p),
	    p->http_port,
	    p->icp.port);
	dump_peer_options(entry, p);
	for (d = p->peer_domain; d; d = d->next) {
	    storeAppendPrintf(entry, "cache_peer_domain %s %s%s\n",
		p->host,
		d->do_ping ? null_string : "!",
		d->domain);
	}
	if (p->access) {
	    snprintf(xname, 128, "cache_peer_access %s", p->host);
	    dump_acl_access(entry, xname, p->access);
	}
	for (t = p->typelist; t; t = t->next) {
	    storeAppendPrintf(entry, "neighbor_type_domain %s %s %s\n",
		p->host,
		peer_type_str(t->type),
		t->domain);
	}
	p = p->next;
    }
}

static void
parse_peer(peer ** head)
{
    char *token = NULL;
    peer *p;
    int i;
    p = cbdataAlloc(peer);
    p->http_port = CACHE_HTTP_PORT;
    p->icp.port = CACHE_ICP_PORT;
    p->weight = 1;
    p->stats.logged_state = PEER_ALIVE;
    if ((token = strtok(NULL, w_space)) == NULL)
	self_destruct();
    p->host = xstrdup(token);
    if ((token = strtok(NULL, w_space)) == NULL)
	self_destruct();
    p->type = parseNeighborType(token);
    i = GetInteger();
    p->http_port = (u_short) i;
    i = GetInteger();
    p->icp.port = (u_short) i;
    while ((token = strtok(NULL, w_space))) {
	if (!strcasecmp(token, "proxy-only")) {
	    p->options.proxy_only = 1;
	} else if (!strcasecmp(token, "no-query")) {
	    p->options.no_query = 1;
	} else if (!strcasecmp(token, "no-digest")) {
	    p->options.no_digest = 1;
	} else if (!strcasecmp(token, "multicast-responder")) {
	    p->options.mcast_responder = 1;
	} else if (!strncasecmp(token, "weight=", 7)) {
	    p->weight = atoi(token + 7);
	} else if (!strcasecmp(token, "closest-only")) {
	    p->options.closest_only = 1;
	} else if (!strncasecmp(token, "ttl=", 4)) {
	    p->mcast.ttl = atoi(token + 4);
	    if (p->mcast.ttl < 0)
		p->mcast.ttl = 0;
	    if (p->mcast.ttl > 128)
		p->mcast.ttl = 128;
	} else if (!strcasecmp(token, "default")) {
	    p->options.default_parent = 1;
	} else if (!strcasecmp(token, "round-robin")) {
	    p->options.roundrobin = 1;
#if USE_HTCP
	} else if (!strcasecmp(token, "htcp")) {
	    p->options.htcp = 1;
#endif
	} else if (!strcasecmp(token, "no-netdb-exchange")) {
	    p->options.no_netdb_exchange = 1;
#if USE_CARP
	} else if (!strncasecmp(token, "carp-load-factor=", 17)) {
	    if (p->type != PEER_PARENT)
		debug(3, 0) ("parse_peer: Ignoring carp-load-factor for non-parent %s/%d\n", p->host, p->http_port);
	    else
		p->carp.load_factor = atof(token + 17);
#endif
#if DELAY_POOLS
	} else if (!strcasecmp(token, "no-delay")) {
	    p->options.no_delay = 1;
#endif
	} else if (!strncasecmp(token, "login=", 6)) {
	    p->login = xstrdup(token + 6);
	    rfc1738_unescape(p->login);
	} else if (!strncasecmp(token, "connect-timeout=", 16)) {
	    p->connect_timeout = atoi(token + 16);
#if USE_CACHE_DIGESTS
	} else if (!strncasecmp(token, "digest-url=", 11)) {
	    p->digest_url = xstrdup(token + 11);
#endif
	} else if (!strcasecmp(token, "allow-miss")) {
	    p->options.allow_miss = 1;
	} else if (!strncasecmp(token, "max-conn=", 9)) {
	    p->max_conn = atoi(token + 9);
	} else {
	    debug(3, 0) ("parse_peer: token='%s'\n", token);
	    self_destruct();
	}
    }
    if (p->weight < 1)
	p->weight = 1;
    p->icp.version = ICP_VERSION_CURRENT;
    p->tcp_up = PEER_TCP_MAGIC_COUNT;
    p->test_fd = -1;
#if USE_CARP
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
    if (p->carp.load_factor) {
	/* calculate this peers hash for use in CARP */
	p->carp.hash = 0;
	for (token = p->host; *token != 0; token++)
	    p->carp.hash += ROTATE_LEFT(p->carp.hash, 19) + (unsigned int) *token;
	p->carp.hash += p->carp.hash * 0x62531965;
	p->carp.hash = ROTATE_LEFT(p->carp.hash, 21);
    }
#endif
#if USE_CACHE_DIGESTS
    if (!p->options.no_digest) {
	p->digest = peerDigestCreate(p);
	cbdataLock(p->digest);	/* so we know when/if digest disappears */
    }
#endif
    while (*head != NULL)
	head = &(*head)->next;
    *head = p;
    Config.npeers++;
    peerClearRR(p);
}

static void
free_peer(peer ** P)
{
    peer *p;
    while ((p = *P) != NULL) {
	*P = p->next;
#if USE_CACHE_DIGESTS
	if (p->digest) {
	    PeerDigest *pd = p->digest;
	    p->digest = NULL;
	    peerDigestNotePeerGone(pd);
	    cbdataUnlock(pd);
	}
#endif
	cbdataFree(p);
    }
    Config.npeers = 0;
}

static void
dump_cachemgrpasswd(StoreEntry * entry, const char *name, cachemgr_passwd * list)
{
    wordlist *w;
    while (list != NULL) {
	if (strcmp(list->passwd, "none") && strcmp(list->passwd, "disable"))
	    storeAppendPrintf(entry, "%s XXXXXXXXXX", name);
	else
	    storeAppendPrintf(entry, "%s %s", name, list->passwd);
	for (w = list->actions; w != NULL; w = w->next) {
	    storeAppendPrintf(entry, " %s", w->key);
	}
	storeAppendPrintf(entry, "\n");
	list = list->next;
    }
}

static void
parse_cachemgrpasswd(cachemgr_passwd ** head)
{
    char *passwd = NULL;
    wordlist *actions = NULL;
    cachemgr_passwd *p;
    cachemgr_passwd **P;
    parse_string(&passwd);
    parse_wordlist(&actions);
    p = xcalloc(1, sizeof(cachemgr_passwd));
    p->passwd = passwd;
    p->actions = actions;
    for (P = head; *P; P = &(*P)->next) {
	/*
	 * See if any of the actions from this line already have a
	 * password from previous lines.  The password checking
	 * routines in cache_manager.c take the the password from
	 * the first cachemgr_passwd struct that contains the
	 * requested action.  Thus, we should warn users who might
	 * think they can have two passwords for the same action.
	 */
	wordlist *w;
	wordlist *u;
	for (w = (*P)->actions; w; w = w->next) {
	    for (u = actions; u; u = u->next) {
		if (strcmp(w->key, u->key))
		    continue;
		debug(0, 0) ("WARNING: action '%s' (line %d) already has a password\n",
		    u->key, config_lineno);
	    }
	}
    }
    *P = p;
}

static void
free_cachemgrpasswd(cachemgr_passwd ** head)
{
    cachemgr_passwd *p;
    while ((p = *head) != NULL) {
	*head = p->next;
	xfree(p->passwd);
	wordlistDestroy(&p->actions);
	xfree(p);
    }
}

static void
dump_denyinfo(StoreEntry * entry, const char *name, acl_deny_info_list * var)
{
    acl_name_list *a;
    while (var != NULL) {
	storeAppendPrintf(entry, "%s %s", name, var->err_page_name);
	for (a = var->acl_list; a != NULL; a = a->next)
	    storeAppendPrintf(entry, " %s", a->name);
	storeAppendPrintf(entry, "\n");
	var = var->next;
    }
}

static void
parse_denyinfo(acl_deny_info_list ** var)
{
    aclParseDenyInfoLine(var);
}

void
free_denyinfo(acl_deny_info_list ** list)
{
    acl_deny_info_list *a = NULL;
    acl_deny_info_list *a_next = NULL;
    acl_name_list *l = NULL;
    acl_name_list *l_next = NULL;
    for (a = *list; a; a = a_next) {
	for (l = a->acl_list; l; l = l_next) {
	    l_next = l->next;
	    memFree(l, MEM_ACL_NAME_LIST);
	    l = NULL;
	}
	a_next = a->next;
	memFree(a, MEM_ACL_DENY_INFO_LIST);
	a = NULL;
    }
    *list = NULL;
}

static void
parse_peer_access(void)
{
    char *host = NULL;
    peer *p;
    if (!(host = strtok(NULL, w_space)))
	self_destruct();
    if ((p = peerFindByName(host)) == NULL) {
	debug(15, 0) ("%s, line %d: No cache_peer '%s'\n",
	    cfg_filename, config_lineno, host);
	return;
    }
    aclParseAccessLine(&p->access);
}

static void
parse_hostdomain(void)
{
    char *host = NULL;
    char *domain = NULL;
    if (!(host = strtok(NULL, w_space)))
	self_destruct();
    while ((domain = strtok(NULL, list_sep))) {
	domain_ping *l = NULL;
	domain_ping **L = NULL;
	peer *p;
	if ((p = peerFindByName(host)) == NULL) {
	    debug(15, 0) ("%s, line %d: No cache_peer '%s'\n",
		cfg_filename, config_lineno, host);
	    continue;
	}
	l = xcalloc(1, sizeof(domain_ping));
	l->do_ping = 1;
	if (*domain == '!') {	/* check for !.edu */
	    l->do_ping = 0;
	    domain++;
	}
	l->domain = xstrdup(domain);
	for (L = &(p->peer_domain); *L; L = &((*L)->next));
	*L = l;
    }
}

static void
parse_hostdomaintype(void)
{
    char *host = NULL;
    char *type = NULL;
    char *domain = NULL;
    if (!(host = strtok(NULL, w_space)))
	self_destruct();
    if (!(type = strtok(NULL, w_space)))
	self_destruct();
    while ((domain = strtok(NULL, list_sep))) {
	domain_type *l = NULL;
	domain_type **L = NULL;
	peer *p;
	if ((p = peerFindByName(host)) == NULL) {
	    debug(15, 0) ("%s, line %d: No cache_peer '%s'\n",
		cfg_filename, config_lineno, host);
	    return;
	}
	l = xcalloc(1, sizeof(domain_type));
	l->type = parseNeighborType(type);
	l->domain = xstrdup(domain);
	for (L = &(p->typelist); *L; L = &((*L)->next));
	*L = l;
    }
}

#if UNUSED_CODE
static void
dump_ushortlist(StoreEntry * entry, const char *name, ushortlist * u)
{
    while (u) {
	storeAppendPrintf(entry, "%s %d\n", name, (int) u->i);
	u = u->next;
    }
}

static int
check_null_ushortlist(ushortlist * u)
{
    return u == NULL;
}

static void
parse_ushortlist(ushortlist ** P)
{
    char *token;
    int i;
    ushortlist *u;
    ushortlist **U;
    while ((token = strtok(NULL, w_space))) {
	if (sscanf(token, "%d", &i) != 1)
	    self_destruct();
	if (i < 0)
	    i = 0;
	u = xcalloc(1, sizeof(ushortlist));
	u->i = (u_short) i;
	for (U = P; *U; U = &(*U)->next);
	*U = u;
    }
}

static void
free_ushortlist(ushortlist ** P)
{
    ushortlist *u;
    while ((u = *P) != NULL) {
	*P = u->next;
	xfree(u);
    }
}
#endif

static void
dump_int(StoreEntry * entry, const char *name, int var)
{
    storeAppendPrintf(entry, "%s %d\n", name, var);
}

void
parse_int(int *var)
{
    int i;
    i = GetInteger();
    *var = i;
}

static void
free_int(int *var)
{
    *var = 0;
}

static void
dump_onoff(StoreEntry * entry, const char *name, int var)
{
    storeAppendPrintf(entry, "%s %s\n", name, var ? "on" : "off");
}

void
parse_onoff(int *var)
{
    char *token = strtok(NULL, w_space);

    if (token == NULL)
	self_destruct();
    if (!strcasecmp(token, "on") || !strcasecmp(token, "enable"))
	*var = 1;
    else
	*var = 0;
}

#define free_onoff free_int

static void
dump_tristate(StoreEntry * entry, const char *name, int var)
{
    const char *state;
    if (var > 0)
	state = "on";
    else if (var < 0)
	state = "warn";
    else
	state = "off";
    storeAppendPrintf(entry, "%s %s\n", name, state);
}

static void
parse_tristate(int *var)
{
    char *token = strtok(NULL, w_space);

    if (token == NULL)
	self_destruct();
    if (!strcasecmp(token, "on") || !strcasecmp(token, "enable"))
	*var = 1;
    else if (!strcasecmp(token, "warn"))
	*var = -1;
    else
	*var = 0;
}

#define free_tristate free_int

static void
dump_refreshpattern(StoreEntry * entry, const char *name, refresh_t * head)
{
    while (head != NULL) {
	storeAppendPrintf(entry, "%s%s %s %d %d%% %d\n",
	    name,
	    head->flags.icase ? " -i" : null_string,
	    head->pattern,
	    (int) head->min / 60,
	    (int) (100.0 * head->pct + 0.5),
	    (int) head->max / 60);
#if HTTP_VIOLATIONS
	if (head->flags.override_expire)
	    storeAppendPrintf(entry, " override-expire");
	if (head->flags.override_lastmod)
	    storeAppendPrintf(entry, " override-lastmod");
	if (head->flags.reload_into_ims)
	    storeAppendPrintf(entry, " reload-into-ims");
	if (head->flags.ignore_reload)
	    storeAppendPrintf(entry, " ignore-reload");
#endif
	storeAppendPrintf(entry, "\n");
	head = head->next;
    }
}

static void
parse_refreshpattern(refresh_t ** head)
{
    char *token;
    char *pattern;
    time_t min = 0;
    double pct = 0.0;
    time_t max = 0;
#if HTTP_VIOLATIONS
    int override_expire = 0;
    int override_lastmod = 0;
    int reload_into_ims = 0;
    int ignore_reload = 0;
#endif
    int i;
    refresh_t *t;
    regex_t comp;
    int errcode;
    int flags = REG_EXTENDED | REG_NOSUB;
    if ((token = strtok(NULL, w_space)) == NULL)
	self_destruct();
    if (strcmp(token, "-i") == 0) {
	flags |= REG_ICASE;
	token = strtok(NULL, w_space);
    } else if (strcmp(token, "+i") == 0) {
	flags &= ~REG_ICASE;
	token = strtok(NULL, w_space);
    }
    if (token == NULL)
	self_destruct();
    pattern = xstrdup(token);
    i = GetInteger();		/* token: min */
    min = (time_t) (i * 60);	/* convert minutes to seconds */
    i = GetInteger();		/* token: pct */
    pct = (double) i / 100.0;
    i = GetInteger();		/* token: max */
    max = (time_t) (i * 60);	/* convert minutes to seconds */
    /* Options */
    while ((token = strtok(NULL, w_space)) != NULL) {
#if HTTP_VIOLATIONS
	if (!strcmp(token, "override-expire"))
	    override_expire = 1;
	else if (!strcmp(token, "override-lastmod"))
	    override_lastmod = 1;
	else if (!strcmp(token, "reload-into-ims")) {
	    reload_into_ims = 1;
	    refresh_nocache_hack = 1;
	    /* tell client_side.c that this is used */
	} else if (!strcmp(token, "ignore-reload")) {
	    ignore_reload = 1;
	    refresh_nocache_hack = 1;
	    /* tell client_side.c that this is used */
	} else
#endif
	    debug(22, 0) ("redreshAddToList: Unknown option '%s': %s\n",
		pattern, token);
    }
    if ((errcode = regcomp(&comp, pattern, flags)) != 0) {
	char errbuf[256];
	regerror(errcode, &comp, errbuf, sizeof errbuf);
	debug(22, 0) ("%s line %d: %s\n",
	    cfg_filename, config_lineno, config_input_line);
	debug(22, 0) ("refreshAddToList: Invalid regular expression '%s': %s\n",
	    pattern, errbuf);
	return;
    }
    pct = pct < 0.0 ? 0.0 : pct;
    max = max < 0 ? 0 : max;
    t = xcalloc(1, sizeof(refresh_t));
    t->pattern = (char *) xstrdup(pattern);
    t->compiled_pattern = comp;
    t->min = min;
    t->pct = pct;
    t->max = max;
    if (flags & REG_ICASE)
	t->flags.icase = 1;
#if HTTP_VIOLATIONS
    if (override_expire)
	t->flags.override_expire = 1;
    if (override_lastmod)
	t->flags.override_lastmod = 1;
    if (reload_into_ims)
	t->flags.reload_into_ims = 1;
    if (ignore_reload)
	t->flags.ignore_reload = 1;
#endif
    t->next = NULL;
    while (*head)
	head = &(*head)->next;
    *head = t;
    safe_free(pattern);
}

#if UNUSED_CODE
static int
check_null_refreshpattern(refresh_t * data)
{
    return data == NULL;
}
#endif

static void
free_refreshpattern(refresh_t ** head)
{
    refresh_t *t;
    while ((t = *head) != NULL) {
	*head = t->next;
	safe_free(t->pattern);
	regfree(&t->compiled_pattern);
	safe_free(t);
    }
}

static void
dump_string(StoreEntry * entry, const char *name, char *var)
{
    if (var != NULL)
	storeAppendPrintf(entry, "%s %s\n", name, var);
}

static void
parse_string(char **var)
{
    char *token = strtok(NULL, w_space);
    safe_free(*var);
    if (token == NULL)
	self_destruct();
    *var = xstrdup(token);
}

static void
free_string(char **var)
{
    safe_free(*var);
}

void
parse_eol(char *volatile *var)
{
    unsigned char *token = (unsigned char *) strtok(NULL, null_string);
    safe_free(*var);
    if (token == NULL)
	self_destruct();
    while (*token && isspace(*token))
	token++;
    if (!*token)
	self_destruct();
    *var = xstrdup((char *) token);
}

#define dump_eol dump_string
#define free_eol free_string


static void
dump_time_t(StoreEntry * entry, const char *name, time_t var)
{
    storeAppendPrintf(entry, "%s %d seconds\n", name, (int) var);
}

void
parse_time_t(time_t * var)
{
    parseTimeLine(var, T_SECOND_STR);
}

static void
free_time_t(time_t * var)
{
    *var = 0;
}

#if UNUSED_CODE
static void
dump_size_t(StoreEntry * entry, const char *name, squid_off_t var)
{
    storeAppendPrintf(entry, "%s %" PRINTF_OFF_T "\n", name, var);
}

#endif

static void
dump_b_size_t(StoreEntry * entry, const char *name, squid_off_t var)
{
    storeAppendPrintf(entry, "%s %" PRINTF_OFF_T " %s\n", name, var, B_BYTES_STR);
}

static void
dump_kb_size_t(StoreEntry * entry, const char *name, squid_off_t var)
{
    storeAppendPrintf(entry, "%s %" PRINTF_OFF_T " %s\n", name, var, B_KBYTES_STR);
}

static void
parse_b_size_t(squid_off_t * var)
{
    parseBytesLine(var, B_BYTES_STR);
}

CBDATA_TYPE(body_size);

static void
parse_body_size_t(dlink_list * bodylist)
{
    body_size *bs;
    CBDATA_INIT_TYPE(body_size);
    bs = cbdataAlloc(body_size);
    bs->maxsize = GetOffT();
    aclParseAccessLine(&bs->access_list);

    dlinkAddTail(bs, &bs->node, bodylist);
}

static void
dump_body_size_t(StoreEntry * entry, const char *name, dlink_list bodylist)
{
    body_size *bs;
    bs = (body_size *) bodylist.head;
    while (bs) {
	acl_list *l;
	acl_access *head = bs->access_list;
	while (head != NULL) {
	    storeAppendPrintf(entry, "%s %" PRINTF_OFF_T " %s", name, bs->maxsize,
		head->allow ? "Allow" : "Deny");
	    for (l = head->acl_list; l != NULL; l = l->next) {
		storeAppendPrintf(entry, " %s%s",
		    l->op ? null_string : "!",
		    l->acl->name);
	    }
	    storeAppendPrintf(entry, "\n");
	    head = head->next;
	}
	bs = (body_size *) bs->node.next;
    }
}

static void
free_body_size_t(dlink_list * bodylist)
{
    body_size *bs, *tempnode;
    bs = (body_size *) bodylist->head;
    while (bs) {
	bs->maxsize = 0;
	aclDestroyAccessList(&bs->access_list);
	tempnode = (body_size *) bs->node.next;
	dlinkDelete(&bs->node, bodylist);
	cbdataFree(bs);
	bs = tempnode;
    }
}

static int
check_null_body_size_t(dlink_list bodylist)
{
    return bodylist.head == NULL;
}

#ifdef HS_FEAT_ICAP

/***************************************************
 * prototypes
 */
static int icap_service_process(icap_service * s);
static void icap_service_init(icap_service * s);
static void icap_service_destroy(icap_service * s);
icap_service *icap_service_lookup(char *name);
static int icap_class_process(icap_class * c);
static void icap_class_destroy(icap_class * c);
static void icap_access_destroy(icap_access * a);
static void dump_wordlist(StoreEntry * entry, const char *name, wordlist * list);
static void icap_class_add(icap_class * c);

/***************************************************
 * icap_service
 */

/* 
 * example:
 * icap_service reqmode_precache 0 icap://192.168.0.1:1344/respmod
 */

static void
parse_icap_service_type(IcapConfig * cfg)
{
    char *token;
    icap_service *A = NULL;
    icap_service *B = NULL;
    icap_service **T = NULL;

    A = cbdataAlloc(icap_service);
    icap_service_init(A);
    parse_string(&A->name);
    parse_string(&A->type_name);
    parse_ushort(&A->bypass);
    parse_string(&A->uri);
    while((token=strtok(NULL,w_space))){
	 if (strcasecmp(token,"no-keep-alive") == 0) {
	     A->keep_alive=0;
         } else {
	     debug(3, 0) ("parse_peer: token='%s'\n", token);
	     self_destruct();
	 }
    }
    debug(3, 5) ("parse_icap_service_type (line %d): %s %s %d %s\n", config_lineno, A->name, A->type_name, A->bypass, A->name);
    if (icap_service_process(A)) {
	/* put into linked list */
	for (B = cfg->service_head, T = &cfg->service_head; B; T = &B->next, B = B->next);
	*T = A;
    } else {
	/* clean up structure */
	debug(3, 0) ("parse_icap_service_type (line %d): skipping %s\n", config_lineno, A->name);
	icap_service_destroy(A);
	cbdataFree(A);
    }

}

static void
dump_icap_service_type(StoreEntry * e, const char *name, IcapConfig cfg)
{
    icap_service *current_node = NULL;

    if (!cfg.service_head) {
	storeAppendPrintf(e, "%s 0\n", name);
	return;
    }
    current_node = cfg.service_head;

    while (current_node) {
	storeAppendPrintf(e, "%s %s %s %d %s", name, current_node->name, current_node->type_name, current_node->bypass, current_node->uri);
	if(current_node->keep_alive == 0){
	     storeAppendPrintf(e," no-keep-alive");
	}
	storeAppendPrintf(e,"\n");
	current_node = current_node->next;
    }

}

static void
free_icap_service_type(IcapConfig * cfg)
{
    while (cfg->service_head) {
	icap_service *current_node = cfg->service_head;
	cfg->service_head = current_node->next;
	icap_service_destroy(current_node);
	cbdataFree(current_node);
    }
}

/* 
 * parse the raw string and cache some parts that are needed later 
 * returns 1 if everything was ok
 */
static int
icap_service_process(icap_service * s)
{
    char *start, *end, *tempEnd;
    char *tailp;
    unsigned int len;
    int port_in_uri, resource_in_uri=0;
    s->type = icapServiceToType(s->type_name);
    if (s->type >= ICAP_SERVICE_MAX) {
	debug(3, 0) ("icap_service_process (line %d): wrong service type %s\n", config_lineno, s->type_name);
	return 0;
    }
    if (s->type == ICAP_SERVICE_REQMOD_PRECACHE)
	s->method = ICAP_METHOD_REQMOD;
    else if (s->type == ICAP_SERVICE_REQMOD_PRECACHE)
	s->method = ICAP_METHOD_REQMOD;
    else if (s->type == ICAP_SERVICE_REQMOD_POSTCACHE)
	s->method = ICAP_METHOD_REQMOD;
    else if (s->type == ICAP_SERVICE_RESPMOD_PRECACHE)
	s->method = ICAP_METHOD_RESPMOD;
    else if (s->type == ICAP_SERVICE_RESPMOD_POSTCACHE)
	s->method = ICAP_METHOD_RESPMOD;
    debug(3, 5) ("icap_service_process (line %d): type=%s\n", config_lineno, icapServiceToStr(s->type));
    if (strncmp(s->uri, "icap://", 7) != 0) {
	debug(3, 0) ("icap_service_process (line %d): wrong uri: %s\n", config_lineno, s->uri);
	return 0;
    }
    start = s->uri + 7;
    if ((end = strchr(start, ':')) != NULL) {
	/* ok */
	port_in_uri = 1;
	debug(3, 5) ("icap_service_process (line %d): port given\n", config_lineno);
    } else {
	/* ok */
	port_in_uri = 0;
	debug(3, 5) ("icap_service_process (line %d): no port given\n", config_lineno);
    }

    if ((tempEnd = strchr(start, '/')) != NULL) {
	/* ok */
	resource_in_uri = 1;
	debug(3, 5) ("icap_service_process (line %d): resource given\n", config_lineno);
	if (end == '\0') {
	    end = tempEnd;
	}
    } else {
	/* ok */
	resource_in_uri = 0;
	debug(3, 5) ("icap_service_process (line %d): no resource given\n", config_lineno);
    }

    tempEnd = strchr(start, '\0');
    if (end == '\0') {
	end = tempEnd;
    }

    len = end - start;
    s->hostname = xstrndup(start, len + 1);
    s->hostname[len] = 0;
    debug(3, 5) ("icap_service_process (line %d): hostname=%s\n", config_lineno, s->hostname);
    start = end;

    if (port_in_uri) {
	start++;		/* skip ':' */
	if (resource_in_uri)
            end = strchr(start, '/');
	else
  	    end = strchr(start, '\0');
	s->port = strtoul(start, &tailp, 0) % 65536;
	if (tailp != end) {
	    debug(3, 0) ("icap_service_process (line %d): wrong service uri (port could not be parsed): %s\n", config_lineno, s->uri);
	    return 0;
	}
	debug(3, 5) ("icap_service_process (line %d): port=%d\n", config_lineno, s->port);
	start = end;
    } else {
	/* no explicit ICAP port; first ask by getservbyname or default to
	 * hardwired port 1344 per ICAP specification section 4.2 */
	struct servent *serv = getservbyname("icap", "tcp");
	if (serv) {
	    s->port = htons(serv->s_port);
	    debug(3, 5) ("icap_service_process (line %d): default port=%d getservbyname(icap,tcp)\n", config_lineno, s->port);
	} else {
	    s->port = 1344;
	    debug(3, 5) ("icap_service_process (line %d): default hardwired port=%d\n", config_lineno, s->port);
	}
    }

    if (resource_in_uri) {
        start++;			/* skip '/' */
        /* the rest is resource name */
        end = strchr(start, '\0');
        len = end - start;
        if (len > 1024) {
	    debug(3, 0) ("icap_service_process (line %d): long resource name (>1024), probably wrong\n", config_lineno);
        }
        s->resource = xstrndup(start, len + 1);
        s->resource[len] = 0;
	debug(3, 5) ("icap_service_process (line %d): service=%s\n", config_lineno, s->resource);
    }

    /* check bypass */
    if ((s->bypass != 0) && (s->bypass != 1)) {
	debug(3, 0) ("icap_service_process (line %d): invalid bypass value\n", config_lineno);
	return 0;
    }
    return 1;
}

/*
 * constructor
 */
static void
icap_service_init(icap_service * s)
{
    s->type = ICAP_SERVICE_MAX;	/* means undefined */
    s->preview = Config.icapcfg.preview_size;
    s->opt = 0;
    s->keep_alive=1;
    s->istag = StringNull;
    s->transfer_preview = StringNull;
    s->transfer_ignore = StringNull;
    s->transfer_complete = StringNull;
}

/*
 * destructor
 * frees only strings, but don't touch the linked list
 */
static void
icap_service_destroy(icap_service * s)
{
    xfree(s->name);
    xfree(s->uri);
    xfree(s->type_name);
    xfree(s->hostname);
    xfree(s->resource);
    assert(s->opt == 0);	/* there should be no opt request running now */
    stringClean(&s->istag);
    stringClean(&s->transfer_preview);
    stringClean(&s->transfer_ignore);
    stringClean(&s->transfer_complete);
}

icap_service *
icap_service_lookup(char *name)
{
    icap_service *iter;
    for (iter = Config.icapcfg.service_head; iter; iter = iter->next) {
	if (!strcmp(name, iter->name)) {
	    return iter;
	}
    }
    return NULL;
}

/***************************************************
 * icap_service_list
 */

static void
icap_service_list_add(icap_service_list ** isl, char * service_name)
{
    icap_service_list **iter;
    icap_service_list *new;
    icap_service      *gbl_service;
    int	              i;
    int		      max_services;

    new = memAllocate(MEM_ICAP_SERVICE_LIST);
    /* Found all services with that name, and add to the array */
    max_services = sizeof(new->services)/sizeof(icap_service *);
    gbl_service = Config.icapcfg.service_head;
    i=0;
    while(gbl_service && i < max_services) {
       if (!strcmp(service_name, gbl_service->name))
	  new->services[i++] = gbl_service;
       gbl_service = gbl_service->next;
    }
    new->nservices = i;

    if (*isl) {
	iter = isl;
	while ((*iter)->next)
	    iter = &((*iter)->next);
	(*iter)->next = new;
    } else {
	*isl = new;
    }
}

/*
 * free the linked list without touching references icap_service
 */
static void
icap_service_list_destroy(icap_service_list * isl)
{
    icap_service_list *current;
    icap_service_list *next;

    current = isl;
    while (current) {
	next = current->next;
	memFree(current, MEM_ICAP_SERVICE_LIST);
	current = next;
    }
}

/***************************************************
 * icap_class
 */
static void
parse_icap_class_type(IcapConfig * cfg)
{
    icap_class *s = NULL;

    s = memAllocate(MEM_ICAP_CLASS);
    parse_string(&s->name);
    parse_wordlist(&s->services);

    if (icap_class_process(s)) {
	/* if ok, put into linked list */
	icap_class_add(s);
    } else {
	/* clean up structure */
	debug(3, 0) ("parse_icap_class_type (line %d): skipping %s\n", config_lineno, s->name);
	icap_class_destroy(s);
	memFree(s, MEM_ICAP_CLASS);
    }
}

static void
dump_icap_class_type(StoreEntry * e, const char *name, IcapConfig cfg)
{
    icap_class *current_node = NULL;
    LOCAL_ARRAY(char, nom, 64);

    if (!cfg.class_head) {
	storeAppendPrintf(e, "%s 0\n", name);
	return;
    }
    current_node = cfg.class_head;

    while (current_node) {
	snprintf(nom, 64, "%s %s", name, current_node->name);
	dump_wordlist(e, nom, current_node->services);
	current_node = current_node->next;
    }
}

static void
free_icap_class_type(IcapConfig * cfg)
{
    while (cfg->class_head) {
	icap_class *current_node = cfg->class_head;
	cfg->class_head = current_node->next;
	icap_class_destroy(current_node);
	memFree(current_node, MEM_ICAP_CLASS);
    }
}

/*
 * process services list, return 1, if at least one service was found
 */
static int
icap_class_process(icap_class * c)
{
    icap_service_list *isl = NULL;
    wordlist *iter;
    icap_service *service;
    /* take services list and build icap_service_list from it */
    for (iter = c->services; iter; iter = iter->next) {
	service = icap_service_lookup(iter->key);
	if (service) {
	    icap_service_list_add(&isl, iter->key);
	} else {
	    debug(3, 0) ("icap_class_process (line %d): skipping service %s in class %s\n", config_lineno, iter->key, c->name);
	}
    }

    if (isl) {
	c->isl = isl;
	return 1;
    }
    return 0;
}

/*
 * search for an icap_class in the global IcapConfig
 * classes with hidden-flag are skipped
 */
static icap_class *
icap_class_lookup(char *name)
{
    icap_class *iter;
    for (iter = Config.icapcfg.class_head; iter; iter = iter->next) {
	if ((!strcmp(name, iter->name)) && (!iter->hidden)) {
	    return iter;
	}
    }
    return NULL;
}

/*
 * adds an icap_class to the global IcapConfig
 */
static void
icap_class_add(icap_class * c)
{
    icap_class *cp = NULL;
    icap_class **t = NULL;
    IcapConfig *cfg = &Config.icapcfg;
    if (c) {
	for (cp = cfg->class_head, t = &cfg->class_head; cp; t = &cp->next, cp = cp->next);
	*t = c;
    }
}

/*
 * free allocated memory inside icap_class
 */
static void
icap_class_destroy(icap_class * c)
{
    xfree(c->name);
    wordlistDestroy(&c->services);
    icap_service_list_destroy(c->isl);
}

/***************************************************
 * icap_access
 */

/* format: icap_access <servicename> {allow|deny} acl, ... */
static void
parse_icap_access_type(IcapConfig * cfg)
{
    icap_access *A = NULL;
    icap_access *B = NULL;
    icap_access **T = NULL;
    icap_service *s = NULL;
    icap_class *c = NULL;
    ushort no_class = 0;

    A = memAllocate(MEM_ICAP_ACCESS);
    parse_string(&A->service_name);

    /* 
     * try to find a class with the given name first. if not found, search 
     * the services. if a service is found, create a new hidden class with 
     * only this service. this is for backward compatibility.
     *
     * the special classname All is allowed only in deny rules, because
     * the class is not used there.
     */
    if (!strcmp(A->service_name, "None")) {
	no_class = 1;
    } else {
	A->class = icap_class_lookup(A->service_name);
	if (!A->class) {
	    s = icap_service_lookup(A->service_name);
	    if (s) {
		c = memAllocate(MEM_ICAP_CLASS);
		c->name = xstrdup("(hidden)");
		c->hidden = 1;
		wordlistAdd(&c->services, A->service_name);
		c->isl = memAllocate(MEM_ICAP_SERVICE_LIST);
		/* FIXME:luc: check what access do */
		c->isl->services[0] = s;
		c->isl->nservices = 1;
		icap_class_add(c);
		A->class = c;
	    } else {
		debug(3, 0) ("parse_icap_access_type (line %d): servicename %s not found. skipping.\n", config_lineno, A->service_name);
		memFree(A, MEM_ICAP_ACCESS);
		return;
	    }
	}
    }

    aclParseAccessLine(&(A->access));
    debug(3, 5) ("parse_icap_access_type (line %d): %s\n", config_lineno, A->service_name);

    /* check that All class is only used in deny rule */
    if (no_class && A->access->allow) {
	memFree(A, MEM_ICAP_ACCESS);
	debug(3, 0) ("parse_icap_access (line %d): special class 'None' only allowed in deny rule. skipping.\n", config_lineno);
	return;
    }
    if (A->access) {
	for (B = cfg->access_head, T = &cfg->access_head; B; T = &B->next, B = B->next);
	*T = A;
    } else {
	debug(3, 0) ("parse_icap_access_type (line %d): invalid line skipped\n", config_lineno);
	memFree(A, MEM_ICAP_ACCESS);
    }
}

static void
dump_icap_access_type(StoreEntry * e, const char *name, IcapConfig cfg)
{
    icap_access *current_node = NULL;
    LOCAL_ARRAY(char, nom, 64);

    if (!cfg.access_head) {
	storeAppendPrintf(e, "%s 0\n", name);
	return;
    }
    current_node = cfg.access_head;

    while (current_node) {
	snprintf(nom, 64, "%s %s", name, current_node->service_name);
	dump_acl_access(e, nom, current_node->access);
	current_node = current_node->next;
    }
}

static void
free_icap_access_type(IcapConfig * cfg)
{
    while (cfg->access_head) {
	icap_access *current_node = cfg->access_head;
	cfg->access_head = current_node->next;
	icap_access_destroy(current_node);
	memFree(current_node, MEM_ICAP_ACCESS);
    }
}

/*
 * destructor
 * frees everything but the linked list
 */
static void
icap_access_destroy(icap_access * a)
{
    xfree(a->service_name);
    aclDestroyAccessList(&a->access);
}

/***************************************************
 * for debugging purposes only
 */
void
dump_icap_config(IcapConfig * cfg)
{
    icap_service *s_iter;
    icap_class *c_iter;
    icap_access *a_iter;
    icap_service_list *isl_iter;
    acl_list *l;
    debug(3, 0) ("IcapConfig: onoff        = %d\n", cfg->onoff);
    debug(3, 0) ("IcapConfig: service_head = %d\n", (int) cfg->service_head);
    debug(3, 0) ("IcapConfig: class_head   = %d\n", (int) cfg->class_head);
    debug(3, 0) ("IcapConfig: access_head  = %d\n", (int) cfg->access_head);

    debug(3, 0) ("IcapConfig: services =\n");
    for (s_iter = cfg->service_head; s_iter; s_iter = s_iter->next) {
	printf("  %s: \n", s_iter->name);
	printf("    bypass   = %d\n", s_iter->bypass);
	printf("    hostname = %s\n", s_iter->hostname);
	printf("    port     = %d\n", s_iter->port);
	printf("    resource = %s\n", s_iter->resource);
    }
    debug(3, 0) ("IcapConfig: classes =\n");
    for (c_iter = cfg->class_head; c_iter; c_iter = c_iter->next) {
	printf("  %s: \n", c_iter->name);
	printf("    services = \n");
	for (isl_iter = c_iter->isl; isl_iter; isl_iter = isl_iter->next) {
	   int i;
	   for (i = 0; i < isl_iter->nservices; i++)
	     printf("      %s\n", isl_iter->services[i]->name);
	}
    }
    debug(3, 0) ("IcapConfig: access =\n");
    for (a_iter = cfg->access_head; a_iter; a_iter = a_iter->next) {
	printf("  service_name  = %s\n", a_iter->service_name);
	printf("    access        = %s", a_iter->access->allow ? "allow" : "deny");
	for (l = a_iter->access->acl_list; l != NULL; l = l->next) {
	    printf(" %s%s",
		l->op ? null_string : "!",
		l->acl->name);
	}
	printf("\n");
    }
}
#endif /* HS_FEAT_ICAP */

static void
parse_kb_size_t(squid_off_t * var)
{
    parseBytesLine(var, B_KBYTES_STR);
}

static void
free_size_t(squid_off_t * var)
{
    *var = 0;
}

#define free_b_size_t free_size_t
#define free_kb_size_t free_size_t
#define free_mb_size_t free_size_t
#define free_gb_size_t free_size_t

static void
dump_ushort(StoreEntry * entry, const char *name, u_short var)
{
    storeAppendPrintf(entry, "%s %d\n", name, var);
}

static void
free_ushort(u_short * u)
{
    *u = 0;
}

static void
parse_ushort(u_short * var)
{
    int i;

    i = GetInteger();
    if (i < 0)
	i = 0;
    *var = (u_short) i;
}

static void
dump_wordlist(StoreEntry * entry, const char *name, wordlist * list)
{
    while (list != NULL) {
	storeAppendPrintf(entry, "%s %s\n", name, list->key);
	list = list->next;
    }
}

void
parse_wordlist(wordlist ** list)
{
    char *token;
    char *t = strtok(NULL, "");
    while ((token = strwordtok(NULL, &t)))
	wordlistAdd(list, token);
}

static int
check_null_wordlist(wordlist * w)
{
    return w == NULL;
}

static int
check_null_acl_access(acl_access * a)
{
    return a == NULL;
}

#define free_wordlist wordlistDestroy

#define free_uri_whitespace free_int

static void
parse_uri_whitespace(int *var)
{
    char *token = strtok(NULL, w_space);
    if (token == NULL)
	self_destruct();
    if (!strcasecmp(token, "strip"))
	*var = URI_WHITESPACE_STRIP;
    else if (!strcasecmp(token, "deny"))
	*var = URI_WHITESPACE_DENY;
    else if (!strcasecmp(token, "allow"))
	*var = URI_WHITESPACE_ALLOW;
    else if (!strcasecmp(token, "encode"))
	*var = URI_WHITESPACE_ENCODE;
    else if (!strcasecmp(token, "chop"))
	*var = URI_WHITESPACE_CHOP;
    else
	self_destruct();
}


static void
dump_uri_whitespace(StoreEntry * entry, const char *name, int var)
{
    const char *s;
    if (var == URI_WHITESPACE_ALLOW)
	s = "allow";
    else if (var == URI_WHITESPACE_ENCODE)
	s = "encode";
    else if (var == URI_WHITESPACE_CHOP)
	s = "chop";
    else if (var == URI_WHITESPACE_DENY)
	s = "deny";
    else
	s = "strip";
    storeAppendPrintf(entry, "%s %s\n", name, s);
}

static void
free_removalpolicy(RemovalPolicySettings ** settings)
{
    if (!*settings)
	return;
    free_string(&(*settings)->type);
    free_wordlist(&(*settings)->args);
    xfree(*settings);
    *settings = NULL;
}

static void
parse_removalpolicy(RemovalPolicySettings ** settings)
{
    if (*settings)
	free_removalpolicy(settings);
    *settings = xcalloc(1, sizeof(**settings));
    parse_string(&(*settings)->type);
    parse_wordlist(&(*settings)->args);
}

static void
dump_removalpolicy(StoreEntry * entry, const char *name, RemovalPolicySettings * settings)
{
    wordlist *args;
    storeAppendPrintf(entry, "%s %s", name, settings->type);
    args = settings->args;
    while (args) {
	storeAppendPrintf(entry, " %s", args->key);
	args = args->next;
    }
    storeAppendPrintf(entry, "\n");
}


#include "cf_parser.h"

peer_t
parseNeighborType(const char *s)
{
    if (!strcasecmp(s, "parent"))
	return PEER_PARENT;
    if (!strcasecmp(s, "neighbor"))
	return PEER_SIBLING;
    if (!strcasecmp(s, "neighbour"))
	return PEER_SIBLING;
    if (!strcasecmp(s, "sibling"))
	return PEER_SIBLING;
    if (!strcasecmp(s, "multicast"))
	return PEER_MULTICAST;
    debug(15, 0) ("WARNING: Unknown neighbor type: %s\n", s);
    return PEER_SIBLING;
}

static void
parse_sockaddr_in_list(sockaddr_in_list ** head)
{
    char *token;
    char *t;
    char *host;
    const struct hostent *hp;
    unsigned short port;
    sockaddr_in_list *s;
    while ((token = strtok(NULL, w_space))) {
	host = NULL;
	port = 0;
	if ((t = strchr(token, ':'))) {
	    /* host:port */
	    host = token;
	    *t = '\0';
	    port = (unsigned short) atoi(t + 1);
	    if (0 == port)
		self_destruct();
	} else if ((port = atoi(token)) > 0) {
	    /* port */
	} else {
	    self_destruct();
	}
	s = xcalloc(1, sizeof(*s));
	s->s.sin_port = htons(port);
	if (NULL == host)
	    s->s.sin_addr = any_addr;
	else if (1 == safe_inet_addr(host, &s->s.sin_addr))
	    (void) 0;
	else if ((hp = gethostbyname(host)))	/* dont use ipcache */
	    s->s.sin_addr = inaddrFromHostent(hp);
	else
	    self_destruct();
	while (*head)
	    head = &(*head)->next;
	*head = s;
    }
}

static void
dump_sockaddr_in_list(StoreEntry * e, const char *n, const sockaddr_in_list * s)
{
    while (s) {
	storeAppendPrintf(e, "%s %s:%d\n",
	    n,
	    inet_ntoa(s->s.sin_addr),
	    ntohs(s->s.sin_port));
	s = s->next;
    }
}

static void
free_sockaddr_in_list(sockaddr_in_list ** head)
{
    sockaddr_in_list *s;
    while ((s = *head) != NULL) {
	*head = s->next;
	xfree(s);
    }
}

static int
check_null_sockaddr_in_list(const sockaddr_in_list * s)
{
    return NULL == s;
}

#if USE_SSL
static void
parse_https_port_list(https_port_list ** head)
{
    char *token;
    char *t;
    char *host;
    const struct hostent *hp;
    unsigned short port;
    https_port_list *s;
    token = strtok(NULL, w_space);
    if (!token)
	self_destruct();
    host = NULL;
    port = 0;
    if ((t = strchr(token, ':'))) {
	/* host:port */
	host = token;
	*t = '\0';
	port = (unsigned short) atoi(t + 1);
	if (0 == port)
	    self_destruct();
    } else if ((port = atoi(token)) > 0) {
	/* port */
    } else {
	self_destruct();
    }
    s = xcalloc(1, sizeof(*s));
    s->s.sin_port = htons(port);
    if (NULL == host)
	s->s.sin_addr = any_addr;
    else if (1 == safe_inet_addr(host, &s->s.sin_addr))
	(void) 0;
    else if ((hp = gethostbyname(host)))	/* dont use ipcache */
	s->s.sin_addr = inaddrFromHostent(hp);
    else
	self_destruct();
    /* parse options ... */
    while ((token = strtok(NULL, w_space))) {
	if (strncmp(token, "cert=", 5) == 0) {
	    safe_free(s->cert);
	    s->cert = xstrdup(token + 5);
	} else if (strncmp(token, "key=", 4) == 0) {
	    safe_free(s->key);
	    s->key = xstrdup(token + 4);
	} else if (strncmp(token, "version=", 8) == 0) {
	    s->version = atoi(token + 8);
	    if (s->version < 1 || s->version > 4)
		self_destruct();
	} else if (strncmp(token, "options=", 8) == 0) {
	    safe_free(s->options);
	    s->options = xstrdup(token + 8);
	} else if (strncmp(token, "cipher=", 7) == 0) {
	    safe_free(s->cipher);
	    s->cipher = xstrdup(token + 7);
	} else {
	    self_destruct();
	}
    }
    while (*head)
	head = &(*head)->next;
    *head = s;
}

static void
dump_https_port_list(StoreEntry * e, const char *n, const https_port_list * s)
{
    while (s) {
	storeAppendPrintf(e, "%s %s:%d cert=\"%s\" key=\"%s\"",
	    n,
	    inet_ntoa(s->s.sin_addr),
	    ntohs(s->s.sin_port),
	    s->cert,
	    s->key);
	if (s->version)
	    storeAppendPrintf(e, " version=%d", s->version);
	if (s->options)
	    storeAppendPrintf(e, " options=%s", s->options);
	if (s->cipher)
	    storeAppendPrintf(e, " cipher=%s", s->cipher);
	storeAppendPrintf(e, "\n");
	s = s->next;
    }
}

static void
free_https_port_list(https_port_list ** head)
{
    https_port_list *s;
    while ((s = *head) != NULL) {
	*head = s->next;
	safe_free(s->cert);
	safe_free(s->key);
	safe_free(s);
    }
}

#if 0
static int
check_null_https_port_list(const https_port_list * s)
{
    return NULL == s;
}
#endif

#endif /* USE_SSL */

void
configFreeMemory(void)
{
    safe_free(Config2.Accel.prefix);
    free_all();
}

void
requirePathnameExists(const char *name, const char *path)
{
    struct stat sb;
    char pathbuf[BUFSIZ];
    assert(path != NULL);
    if (Config.chroot_dir) {
	snprintf(pathbuf, BUFSIZ, "%s/%s", Config.chroot_dir, path);
	path = pathbuf;
    }
    if (stat(path, &sb) < 0)
	fatalf("%s %s: %s", name, path, xstrerror());
}

char *
strtokFile(void)
{
    static int fromFile = 0;
    static FILE *wordFile = NULL;

    char *t, *fn;
    LOCAL_ARRAY(char, buf, 256);

  strtok_again:
    if (!fromFile) {
	t = (strtok(NULL, w_space));
	if (!t || *t == '#') {
	    return NULL;
	} else if (*t == '\"' || *t == '\'') {
	    /* quote found, start reading from file */
	    fn = ++t;
	    while (*t && *t != '\"' && *t != '\'')
		t++;
	    *t = '\0';
	    if ((wordFile = fopen(fn, "r")) == NULL) {
		debug(28, 0) ("strtokFile: %s not found\n", fn);
		return (NULL);
	    }
#if defined(_SQUID_MSWIN_) || defined(_SQUID_CYGWIN_)
	    setmode(fileno(wordFile), O_TEXT);
#endif
	    fromFile = 1;
	} else {
	    return t;
	}
    }
    /* fromFile */
    if (fgets(buf, 256, wordFile) == NULL) {
	/* stop reading from file */
	fclose(wordFile);
	wordFile = NULL;
	fromFile = 0;
	goto strtok_again;
    } else {
	char *t2, *t3;
	t = buf;
	/* skip leading and trailing white space */
	t += strspn(buf, w_space);
	t2 = t + strcspn(t, w_space);
	t3 = t2 + strspn(t2, w_space);
	while (*t3 && *t3 != '#') {
	    t2 = t3 + strcspn(t3, w_space);
	    t3 = t2 + strspn(t2, w_space);
	}
	*t2 = '\0';
	/* skip comments */
	if (*t == '#')
	    goto strtok_again;
	/* skip blank lines */
	if (!*t)
	    goto strtok_again;
	return t;
    }
}
