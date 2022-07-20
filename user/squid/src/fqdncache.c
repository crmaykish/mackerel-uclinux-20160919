
/*
 * $Id: fqdncache.c,v 1.149.2.7 2005/02/21 16:59:45 hno Exp $
 *
 * DEBUG: section 35    FQDN Cache
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

#define FQDN_LOW_WATER       90
#define FQDN_HIGH_WATER      95

typedef struct _fqdncache_entry fqdncache_entry;

struct _fqdncache_entry {
    hash_link hash;		/* must be first */
    time_t lastref;
    time_t expires;
    unsigned char name_count;
    char *names[FQDN_MAX_NAMES + 1];
    FQDNH *handler;
    void *handlerData;
    char *error_message;
    struct timeval request_time;
    dlink_node lru;
    unsigned short locks;
    struct {
	unsigned int negcached:1;
	unsigned int fromhosts:1;
    } flags;
};

static struct {
    int requests;
    int replies;
    int hits;
    int misses;
    int negative_hits;
    int errors;
    int ghba_calls;		/* # calls to blocking gethostbyaddr() */
} FqdncacheStats;

static dlink_list lru_list;

#if USE_DNSSERVERS
static HLPCB fqdncacheHandleReply;
static fqdncache_entry *fqdncacheParse(fqdncache_entry *, const char *buf);
#else
static IDNSCB fqdncacheHandleReply;
static fqdncache_entry *fqdncacheParse(fqdncache_entry *, rfc1035_rr *, int, const char *error_message);
#endif
static void fqdncacheRelease(fqdncache_entry *);
static fqdncache_entry *fqdncacheCreateEntry(const char *name);
static void fqdncacheCallback(fqdncache_entry *);
static fqdncache_entry *fqdncache_get(const char *);
static FQDNH dummy_handler;
static int fqdncacheExpiredEntry(const fqdncache_entry *);
static void fqdncacheLockEntry(fqdncache_entry * f);
static void fqdncacheUnlockEntry(fqdncache_entry * f);
static FREE fqdncacheFreeEntry;
static void fqdncacheAddEntry(fqdncache_entry * f);

static hash_table *fqdn_table = NULL;

static long fqdncache_low = 180;
static long fqdncache_high = 200;

/* removes the given fqdncache entry */
static void
fqdncacheRelease(fqdncache_entry * f)
{
    int k;
    hash_remove_link(fqdn_table, (hash_link *) f);
    for (k = 0; k < (int) f->name_count; k++)
	safe_free(f->names[k]);
    debug(35, 5) ("fqdncacheRelease: Released FQDN record for '%s'.\n",
	hashKeyStr(&f->hash));
    dlinkDelete(&f->lru, &lru_list);
    safe_free(f->hash.key);
    safe_free(f->error_message);
    memFree(f, MEM_FQDNCACHE_ENTRY);
}

/* return match for given name */
static fqdncache_entry *
fqdncache_get(const char *name)
{
    hash_link *e;
    static fqdncache_entry *f;
    f = NULL;
    if (fqdn_table) {
	if ((e = hash_lookup(fqdn_table, name)) != NULL)
	    f = (fqdncache_entry *) e;
    }
    return f;
}

static int
fqdncacheExpiredEntry(const fqdncache_entry * f)
{
    /* all static entries are locked, so this takes care of them too */
    if (f->locks != 0)
	return 0;
    if (f->expires > squid_curtime)
	return 0;
    return 1;
}

void
fqdncache_purgelru(void *notused)
{
    dlink_node *m;
    dlink_node *prev = NULL;
    fqdncache_entry *f;
    int removed = 0;
    eventAdd("fqdncache_purgelru", fqdncache_purgelru, NULL, 10.0, 1);
    for (m = lru_list.tail; m; m = prev) {
	if (memInUse(MEM_FQDNCACHE_ENTRY) < fqdncache_low)
	    break;
	prev = m->prev;
	f = m->data;
	if (f->locks != 0)
	    continue;
	fqdncacheRelease(f);
	removed++;
    }
    debug(35, 9) ("fqdncache_purgelru: removed %d entries\n", removed);
}

static void
purge_entries_fromhosts(void)
{
    dlink_node *m = lru_list.head;
    fqdncache_entry *i = NULL;
    fqdncache_entry *t;
    while (m) {
	if (i != NULL) {	/* need to delay deletion */
	    fqdncacheRelease(i);	/* we just override locks */
	    i = NULL;
	}
	t = m->data;
	if (t->flags.fromhosts)
	    i = t;
	m = m->next;
    }
    if (i != NULL)
	fqdncacheRelease(i);
}

/* create blank fqdncache_entry */
static fqdncache_entry *
fqdncacheCreateEntry(const char *name)
{
    static fqdncache_entry *f;
    f = memAllocate(MEM_FQDNCACHE_ENTRY);
    f->hash.key = xstrdup(name);
    f->expires = squid_curtime + Config.negativeDnsTtl;
    return f;
}

static void
fqdncacheAddEntry(fqdncache_entry * f)
{
    fqdncache_entry *e = (fqdncache_entry *) hash_lookup(fqdn_table, f->hash.key);
    if (NULL != e) {
	/* avoid collision */
	if (f->flags.negcached && !e->flags.negcached && e->expires > squid_curtime) {
	    /* Don't waste good information */
	    fqdncacheFreeEntry(f);
	    return;
	}
	fqdncacheRelease(e);
    }
    hash_join(fqdn_table, &f->hash);
    dlinkAdd(f, &f->lru, &lru_list);
    f->lastref = squid_curtime;
}

/* walks down the pending list, calling handlers */
static void
fqdncacheCallback(fqdncache_entry * f)
{
    FQDNH *handler = f->handler;
    void *handlerData = f->handlerData;
    f->lastref = squid_curtime;
    if (NULL == handler)
	return;
    fqdncacheLockEntry(f);
    f->handler = NULL;
    f->handlerData = NULL;
    if (cbdataValid(handlerData)) {
	dns_error_message = f->error_message;
	handler(f->flags.negcached ? NULL : f->names[0], handlerData);
    }
    cbdataUnlock(handlerData);
    fqdncacheUnlockEntry(f);
}

static fqdncache_entry *
#if USE_DNSSERVERS
fqdncacheParse(fqdncache_entry * f, const char *inbuf)
{
    LOCAL_ARRAY(char, buf, DNS_INBUF_SZ);
    char *token;
    int ttl;
    const char *name = (const char *) f->hash.key;
    f->expires = squid_curtime + Config.negativeDnsTtl;
    f->flags.negcached = 1;
    if (inbuf == NULL) {
	debug(35, 1) ("fqdncacheParse: Got <NULL> reply in response to '%s'\n", name);
	f->error_message = xstrdup("Internal Error");
	return f;
    }
    xstrncpy(buf, inbuf, DNS_INBUF_SZ);
    debug(35, 5) ("fqdncacheParse: parsing: {%s}\n", buf);
    token = strtok(buf, w_space);
    if (NULL == token) {
	debug(35, 1) ("fqdncacheParse: Got <NULL>, expecting '$name' in response to '%s'\n", name);
	f->error_message = xstrdup("Internal Error");
	return f;
    }
    if (0 == strcmp(token, "$fail")) {
	token = strtok(NULL, "\n");
	assert(NULL != token);
	f->error_message = xstrdup(token);
	return f;
    }
    if (0 != strcmp(token, "$name")) {
	debug(35, 1) ("fqdncacheParse: Got '%s', expecting '$name' in response to '%s'\n", inbuf, name);
	f->error_message = xstrdup("Internal Error");
	return f;
    }
    token = strtok(NULL, w_space);
    if (NULL == token) {
	debug(35, 1) ("fqdncacheParse: Got '%s', expecting TTL in response to '%s'\n", inbuf, name);
	f->error_message = xstrdup("Internal Error");
	return f;
    }
    f->flags.negcached = 0;
    ttl = atoi(token);
    if (ttl == 0 || ttl > Config.positiveDnsTtl)
	ttl = Config.positiveDnsTtl;
    if (ttl < Config.negativeDnsTtl)
	ttl = Config.negativeDnsTtl;
    f->expires = squid_curtime + ttl;
    token = strtok(NULL, w_space);
    if (NULL != token) {
	f->names[0] = xstrdup(token);
	f->name_count = 1;
    }
    return f;
}
#else
fqdncacheParse(fqdncache_entry * f, rfc1035_rr * answers, int nr, const char *error_message)
{
    int k;
    int ttl = 0;
    const char *name = (const char *) f->hash.key;
    f->expires = squid_curtime + Config.negativeDnsTtl;
    f->flags.negcached = 1;
    if (nr < 0) {
	debug(35, 3) ("fqdncacheParse: Lookup of '%s' failed (%s)\n", name, error_message);
	f->error_message = xstrdup(error_message);
	return f;
    }
    if (nr == 0) {
	debug(35, 3) ("fqdncacheParse: No DNS records for '%s'\n", name);
	f->error_message = xstrdup("No DNS records");
	return f;
    }
    debug(35, 3) ("fqdncacheParse: %d answers for '%s'\n", nr, name);
    assert(answers);
    for (k = 0; k < nr; k++) {
	if (answers[k].type != RFC1035_TYPE_PTR)
	    continue;
	if (answers[k].class != RFC1035_CLASS_IN)
	    continue;
	if (!answers[k].rdata[0]) {
	    debug(35, 2) ("fqdncacheParse: blank PTR record for '%s'\n", name);
	    continue;
	}
	if (strchr(answers[k].rdata, ' ')) {
	    debug(35, 2) ("fqdncacheParse: invalid PTR record '%s' for '%s'\n", answers[k].rdata, name);
	    continue;
	}
	f->names[f->name_count++] = xstrdup(answers[k].rdata);
	if (ttl == 0 || answers[k].ttl < ttl)
	    ttl = answers[k].ttl;
	if (f->name_count >= FQDN_MAX_NAMES)
	    break;
    }
    if (f->name_count == 0) {
	debug(35, 1) ("fqdncacheParse: No PTR record for '%s'\n", name);
	f->error_message = xstrdup("No PTR record");
	return f;
    }
    if (ttl == 0 || ttl > Config.positiveDnsTtl)
	ttl = Config.positiveDnsTtl;
    if (ttl < Config.negativeDnsTtl)
	ttl = Config.negativeDnsTtl;
    f->expires = squid_curtime + ttl;
    f->flags.negcached = 0;
    return f;
}
#endif

static void
#if USE_DNSSERVERS
fqdncacheHandleReply(void *data, char *reply)
#else
fqdncacheHandleReply(void *data, rfc1035_rr * answers, int na, const char *error_message)
#endif
{
    int n;
    generic_cbdata *c = data;
    fqdncache_entry *f = c->data;
    cbdataFree(c);
    c = NULL;
    n = ++FqdncacheStats.replies;
    statHistCount(&statCounter.dns.svc_time,
	tvSubMsec(f->request_time, current_time));
#if USE_DNSSERVERS
    fqdncacheParse(f, reply);
#else
    fqdncacheParse(f, answers, na, error_message);
#endif
    fqdncacheAddEntry(f);
    fqdncacheCallback(f);
}

void
fqdncache_nbgethostbyaddr(struct in_addr addr, FQDNH * handler, void *handlerData)
{
    fqdncache_entry *f = NULL;
    char *name = inet_ntoa(addr);
    generic_cbdata *c;
    assert(handler);
    debug(35, 4) ("fqdncache_nbgethostbyaddr: Name '%s'.\n", name);
    FqdncacheStats.requests++;
    if (name == NULL || name[0] == '\0') {
	debug(35, 4) ("fqdncache_nbgethostbyaddr: Invalid name!\n");
	dns_error_message = "Invalid hostname";
	handler(NULL, handlerData);
	return;
    }
    f = fqdncache_get(name);
    if (NULL == f) {
	/* miss */
	(void) 0;
    } else if (fqdncacheExpiredEntry(f)) {
	/* hit, but expired -- bummer */
	fqdncacheRelease(f);
	f = NULL;
    } else {
	/* hit */
	debug(35, 4) ("fqdncache_nbgethostbyaddr: HIT for '%s'\n", name);
	if (f->flags.negcached)
	    FqdncacheStats.negative_hits++;
	else
	    FqdncacheStats.hits++;
	f->handler = handler;
	f->handlerData = handlerData;
	cbdataLock(handlerData);
	fqdncacheCallback(f);
	return;
    }

    debug(35, 5) ("fqdncache_nbgethostbyaddr: MISS for '%s'\n", name);
    FqdncacheStats.misses++;
    f = fqdncacheCreateEntry(name);
    f->handler = handler;
    f->handlerData = handlerData;
    cbdataLock(handlerData);
    f->request_time = current_time;
    c = cbdataAlloc(generic_cbdata);
    c->data = f;
#if USE_DNSSERVERS
    dnsSubmit(hashKeyStr(&f->hash), fqdncacheHandleReply, c);
#else
    idnsPTRLookup(addr, fqdncacheHandleReply, c);
#endif
}

/* initialize the fqdncache */
void
fqdncache_init(void)
{
    int n;
    if (fqdn_table)
	return;
    debug(35, 3) ("Initializing FQDN Cache...\n");
    memset(&FqdncacheStats, '\0', sizeof(FqdncacheStats));
    memset(&lru_list, '\0', sizeof(lru_list));
    fqdncache_high = (long) (((float) Config.fqdncache.size *
	    (float) FQDN_HIGH_WATER) / (float) 100);
    fqdncache_low = (long) (((float) Config.fqdncache.size *
	    (float) FQDN_LOW_WATER) / (float) 100);
    n = hashPrime(fqdncache_high / 4);
    fqdn_table = hash_create((HASHCMP *) strcmp, n, hash4);
    cachemgrRegister("fqdncache",
	"FQDN Cache Stats and Contents",
	fqdnStats, 0, 1);
    memDataInit(MEM_FQDNCACHE_ENTRY, "fqdncache_entry",
	sizeof(fqdncache_entry), 0);
}

const char *
fqdncache_gethostbyaddr(struct in_addr addr, int flags)
{
    char *name = inet_ntoa(addr);
    fqdncache_entry *f = NULL;
    struct in_addr ip;
    assert(name);
    FqdncacheStats.requests++;
    f = fqdncache_get(name);
    if (NULL == f) {
	(void) 0;
    } else if (fqdncacheExpiredEntry(f)) {
	fqdncacheRelease(f);
	f = NULL;
    } else if (f->flags.negcached) {
	FqdncacheStats.negative_hits++;
	dns_error_message = f->error_message;
	return NULL;
    } else {
	FqdncacheStats.hits++;
	f->lastref = squid_curtime;
	dns_error_message = f->error_message;
	return f->names[0];
    }
    dns_error_message = NULL;
    /* check if it's already a FQDN address in text form. */
    if (!safe_inet_addr(name, &ip))
	return name;
    FqdncacheStats.misses++;
    if (flags & FQDN_LOOKUP_IF_MISS)
	fqdncache_nbgethostbyaddr(addr, dummy_handler, NULL);
    return NULL;
}


/* process objects list */
void
fqdnStats(StoreEntry * sentry)
{
    fqdncache_entry *f = NULL;
    int k;
    int ttl;
    if (fqdn_table == NULL)
	return;
    storeAppendPrintf(sentry, "FQDN Cache Statistics:\n");
    storeAppendPrintf(sentry, "FQDNcache Entries: %d\n",
	memInUse(MEM_FQDNCACHE_ENTRY));
    storeAppendPrintf(sentry, "FQDNcache Requests: %d\n",
	FqdncacheStats.requests);
    storeAppendPrintf(sentry, "FQDNcache Hits: %d\n",
	FqdncacheStats.hits);
    storeAppendPrintf(sentry, "FQDNcache Negative Hits: %d\n",
	FqdncacheStats.negative_hits);
    storeAppendPrintf(sentry, "FQDNcache Misses: %d\n",
	FqdncacheStats.misses);
    storeAppendPrintf(sentry, "Blocking calls to gethostbyaddr(): %d\n",
	FqdncacheStats.ghba_calls);
    storeAppendPrintf(sentry, "FQDN Cache Contents:\n\n");
    storeAppendPrintf(sentry, "%-15.15s %3s %3s %3s %s\n",
	"Address", "Flg", "TTL", "Cnt", "Hostnames");
    hash_first(fqdn_table);
    while ((f = (fqdncache_entry *) hash_next(fqdn_table))) {
	ttl = (f->flags.fromhosts ? -1 : (f->expires - squid_curtime));
	storeAppendPrintf(sentry, "%-15.15s  %c%c %3.3d % 3d",
	    hashKeyStr(&f->hash),
	    f->flags.negcached ? 'N' : ' ',
	    f->flags.fromhosts ? 'H' : ' ',
	    ttl,
	    (int) f->name_count);
	for (k = 0; k < (int) f->name_count; k++)
	    storeAppendPrintf(sentry, " %s", f->names[k]);
	storeAppendPrintf(sentry, "\n");
    }
}

static void
dummy_handler(const char *bufnotused, void *datanotused)
{
    return;
}

const char *
fqdnFromAddr(struct in_addr addr)
{
    const char *n;
    static char buf[32];
    if (Config.onoff.log_fqdn && (n = fqdncache_gethostbyaddr(addr, 0)))
	return n;
    xstrncpy(buf, inet_ntoa(addr), 32);
    return buf;
}

static void
fqdncacheLockEntry(fqdncache_entry * f)
{
    if (f->locks++ == 0) {
	dlinkDelete(&f->lru, &lru_list);
	dlinkAdd(f, &f->lru, &lru_list);
    }
}

static void
fqdncacheUnlockEntry(fqdncache_entry * f)
{
    assert(f->locks > 0);
    f->locks--;
    if (fqdncacheExpiredEntry(f))
	fqdncacheRelease(f);
}

static void
fqdncacheFreeEntry(void *data)
{
    fqdncache_entry *f = data;
    int k;
    for (k = 0; k < (int) f->name_count; k++)
	safe_free(f->names[k]);
    safe_free(f->hash.key);
    safe_free(f->error_message);
    memFree(f, MEM_FQDNCACHE_ENTRY);
}

void
fqdncacheFreeMemory(void)
{
    hashFreeItems(fqdn_table, fqdncacheFreeEntry);
    hashFreeMemory(fqdn_table);
    fqdn_table = NULL;
}

/* Recalculate FQDN cache size upon reconfigure */
void
fqdncache_restart(void)
{
    fqdncache_high = (long) (((float) Config.fqdncache.size *
	    (float) FQDN_HIGH_WATER) / (float) 100);
    fqdncache_low = (long) (((float) Config.fqdncache.size *
	    (float) FQDN_LOW_WATER) / (float) 100);
    purge_entries_fromhosts();
}

/*
 *  adds a "static" entry from /etc/hosts.  the worldist is to be
 *  managed by the caller, including pointed-to strings
 */
void
fqdncacheAddEntryFromHosts(char *addr, wordlist * hostnames)
{
    fqdncache_entry *fce;
    int j = 0;
    if ((fce = fqdncache_get(addr))) {
	if (1 == fce->flags.fromhosts) {
	    fqdncacheUnlockEntry(fce);
	} else if (fce->locks > 0) {
	    debug(35, 1) ("fqdncacheAddEntryFromHosts: can't add static entry for locked address '%s'\n", addr);
	    return;
	} else {
	    fqdncacheRelease(fce);
	}
    }
    fce = fqdncacheCreateEntry(addr);
    while (hostnames) {
	fce->names[j] = xstrdup(hostnames->key);
	j++;
	hostnames = hostnames->next;
	if (j >= FQDN_MAX_NAMES)
	    break;
    }
    fce->name_count = j;
    fce->names[j] = NULL;	/* it's safe */
    fce->flags.fromhosts = 1;
    fqdncacheAddEntry(fce);
    fqdncacheLockEntry(fce);
}


#ifdef SQUID_SNMP
/*
 * The function to return the fqdn statistics via SNMP
 */

variable_list *
snmp_netFqdnFn(variable_list * Var, snint * ErrP)
{
    variable_list *Answer = NULL;
    debug(49, 5) ("snmp_netFqdnFn: Processing request:\n");
    snmpDebugOid(5, Var->name, Var->name_length);
    *ErrP = SNMP_ERR_NOERROR;
    switch (Var->name[LEN_SQ_NET + 1]) {
    case FQDN_ENT:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    memInUse(MEM_FQDNCACHE_ENTRY),
	    SMI_GAUGE32);
	break;
    case FQDN_REQ:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    FqdncacheStats.requests,
	    SMI_COUNTER32);
	break;
    case FQDN_HITS:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    FqdncacheStats.hits,
	    SMI_COUNTER32);
	break;
    case FQDN_PENDHIT:
	/* this is now worthless */
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    0,
	    SMI_GAUGE32);
	break;
    case FQDN_NEGHIT:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    FqdncacheStats.negative_hits,
	    SMI_COUNTER32);
	break;
    case FQDN_MISS:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    FqdncacheStats.misses,
	    SMI_COUNTER32);
	break;
    case FQDN_GHBN:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    FqdncacheStats.ghba_calls,
	    SMI_COUNTER32);
	break;
    default:
	*ErrP = SNMP_ERR_NOSUCHNAME;
	break;
    }
    return Answer;
}

#endif /*SQUID_SNMP */
