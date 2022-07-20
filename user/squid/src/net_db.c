
/*
 * $Id: net_db.c,v 1.158.2.9 2005/03/26 02:50:53 hno Exp $
 *
 * DEBUG: section 38    Network Measurement Database
 * AUTHOR: Duane Wessels
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

#if USE_ICMP

typedef struct {
    peer *p;
    StoreEntry *e;
    store_client *sc;
    request_t *r;
    squid_off_t seen;
    squid_off_t used;
    size_t buf_sz;
    char *buf;
} netdbExchangeState;

static hash_table *addr_table = NULL;
static hash_table *host_table = NULL;

static struct in_addr networkFromInaddr(struct in_addr a);
static void netdbRelease(netdbEntry * n);
static void netdbHashInsert(netdbEntry * n, struct in_addr addr);
static void netdbHashDelete(const char *key);
static void netdbHostInsert(netdbEntry * n, const char *hostname);
static void netdbHostDelete(const net_db_name * x);
static void netdbPurgeLRU(void);
static netdbEntry *netdbLookupHost(const char *key);
static net_db_peer *netdbPeerByName(const netdbEntry * n, const char *);
static net_db_peer *netdbPeerAdd(netdbEntry * n, peer * e);
static const char *netdbPeerName(const char *name);
static IPH netdbSendPing;
static QS sortPeerByRtt;
static QS sortByRtt;
static QS netdbLRU;
static FREE netdbFreeNameEntry;
static FREE netdbFreeNetdbEntry;
static STCB netdbExchangeHandleReply;
static void netdbExchangeDone(void *);

/* We have to keep a local list of peer names.  The Peers structure
 * gets freed during a reconfigure.  We want this database to
 * remain persisitent, so _net_db_peer->peername points into this
 * linked list */
static wordlist *peer_names = NULL;

static void
netdbHashInsert(netdbEntry * n, struct in_addr addr)
{
    xstrncpy(n->network, inet_ntoa(networkFromInaddr(addr)), 16);
    n->hash.key = n->network;
    assert(hash_lookup(addr_table, n->network) == NULL);
    hash_join(addr_table, &n->hash);
}

static void
netdbHashDelete(const char *key)
{
    hash_link *hptr = hash_lookup(addr_table, key);
    if (hptr == NULL) {
	debug_trap("netdbHashDelete: key not found");
	return;
    }
    hash_remove_link(addr_table, hptr);
}

static void
netdbHostInsert(netdbEntry * n, const char *hostname)
{
    net_db_name *x = memAllocate(MEM_NET_DB_NAME);
    x->hash.key = xstrdup(hostname);
    x->next = n->hosts;
    n->hosts = x;
    x->net_db_entry = n;
    assert(hash_lookup(host_table, hostname) == NULL);
    hash_join(host_table, &x->hash);
    n->link_count++;
}

static void
netdbHostDelete(const net_db_name * x)
{
    netdbEntry *n;
    net_db_name **X;
    assert(x != NULL);
    assert(x->net_db_entry != NULL);
    n = x->net_db_entry;
    n->link_count--;
    for (X = &n->hosts; *X; X = &(*X)->next) {
	if (*X == x) {
	    *X = x->next;
	    break;
	}
    }
    hash_remove_link(host_table, (hash_link *) x);
    xfree(x->hash.key);
    memFree((void *) x, MEM_NET_DB_NAME);
}

static netdbEntry *
netdbLookupHost(const char *key)
{
    net_db_name *x = (net_db_name *) hash_lookup(host_table, key);
    return x ? x->net_db_entry : NULL;
}

static void
netdbRelease(netdbEntry * n)
{
    net_db_name *x;
    net_db_name *next;
    for (x = n->hosts; x; x = next) {
	next = x->next;
	netdbHostDelete(x);
    }
    n->hosts = NULL;
    safe_free(n->peers);
    n->peers = NULL;
    n->n_peers = 0;
    n->n_peers_alloc = 0;
    if (n->link_count == 0) {
	netdbHashDelete(n->network);
	memFree(n, MEM_NETDBENTRY);
    }
}

static int
netdbLRU(const void *A, const void *B)
{
    const netdbEntry *const *n1 = A;
    const netdbEntry *const *n2 = B;
    if ((*n1)->last_use_time > (*n2)->last_use_time)
	return (1);
    if ((*n1)->last_use_time < (*n2)->last_use_time)
	return (-1);
    return (0);
}

static void
netdbPurgeLRU(void)
{
    netdbEntry *n;
    netdbEntry **list;
    int k = 0;
    int list_count = 0;
    int removed = 0;
    list = xcalloc(memInUse(MEM_NETDBENTRY), sizeof(netdbEntry *));
    hash_first(addr_table);
    while ((n = (netdbEntry *) hash_next(addr_table))) {
	assert(list_count < memInUse(MEM_NETDBENTRY));
	*(list + list_count) = n;
	list_count++;
    }
    qsort((char *) list,
	list_count,
	sizeof(netdbEntry *),
	netdbLRU);
    for (k = 0; k < list_count; k++) {
	if (memInUse(MEM_NETDBENTRY) < Config.Netdb.low)
	    break;
	netdbRelease(*(list + k));
	removed++;
    }
    xfree(list);
}

static netdbEntry *
netdbLookupAddr(struct in_addr addr)
{
    netdbEntry *n;
    char *key = inet_ntoa(networkFromInaddr(addr));
    n = (netdbEntry *) hash_lookup(addr_table, key);
    return n;
}

static netdbEntry *
netdbAdd(struct in_addr addr)
{
    netdbEntry *n;
    if (memInUse(MEM_NETDBENTRY) > Config.Netdb.high)
	netdbPurgeLRU();
    if ((n = netdbLookupAddr(addr)) == NULL) {
	n = memAllocate(MEM_NETDBENTRY);
	netdbHashInsert(n, addr);
    }
    return n;
}

static void
netdbSendPing(const ipcache_addrs * ia, void *data)
{
    struct in_addr addr;
    char *hostname = ((generic_cbdata *) data)->data;
    netdbEntry *n;
    netdbEntry *na;
    net_db_name *x;
    net_db_name **X;
    cbdataFree(data);
    if (ia == NULL) {
	xfree(hostname);
	return;
    }
    addr = ia->in_addrs[ia->cur];
    if ((n = netdbLookupHost(hostname)) == NULL) {
	n = netdbAdd(addr);
	netdbHostInsert(n, hostname);
    } else if ((na = netdbLookupAddr(addr)) != n) {
	/*
	 *hostname moved from 'network n' to 'network na'!
	 */
	if (na == NULL)
	    na = netdbAdd(addr);
	debug(38, 3) ("netdbSendPing: %s moved from %s to %s\n",
	    hostname, n->network, na->network);
	x = (net_db_name *) hash_lookup(host_table, hostname);
	if (x == NULL) {
	    debug(38, 1) ("netdbSendPing: net_db_name list bug: %s not found\n", hostname);
	    xfree(hostname);
	    return;
	}
	/* remove net_db_name from 'network n' linked list */
	for (X = &n->hosts; *X; X = &(*X)->next) {
	    if (*X == x) {
		*X = x->next;
		break;
	    }
	}
	n->link_count--;
	/* point to 'network na' from host entry */
	x->net_db_entry = na;
	/* link net_db_name to 'network na' */
	x->next = na->hosts;
	na->hosts = x;
	na->link_count++;
	n = na;
    }
    if (n->next_ping_time <= squid_curtime) {
	debug(38, 3) ("netdbSendPing: pinging %s\n", hostname);
	icmpDomainPing(addr, hostname);
	n->pings_sent++;
	n->next_ping_time = squid_curtime + Config.Netdb.period;
	n->last_use_time = squid_curtime;
    }
    xfree(hostname);
}

static struct in_addr
networkFromInaddr(struct in_addr a)
{
    struct in_addr b;
    b.s_addr = ntohl(a.s_addr);
#if USE_CLASSFUL
    if (IN_CLASSC(b.s_addr))
	b.s_addr &= IN_CLASSC_NET;
    else if (IN_CLASSB(b.s_addr))
	b.s_addr &= IN_CLASSB_NET;
    else if (IN_CLASSA(b.s_addr))
	b.s_addr &= IN_CLASSA_NET;
#else
    /* use /24 for everything */
    b.s_addr &= IN_CLASSC_NET;
#endif
    b.s_addr = htonl(b.s_addr);
    return b;
}

static int
sortByRtt(const void *A, const void *B)
{
    const netdbEntry *const *n1 = A;
    const netdbEntry *const *n2 = B;
    if ((*n1)->rtt > (*n2)->rtt)
	return 1;
    else if ((*n1)->rtt < (*n2)->rtt)
	return -1;
    else
	return 0;
}

static net_db_peer *
netdbPeerByName(const netdbEntry * n, const char *peername)
{
    int i;
    net_db_peer *p = n->peers;
    for (i = 0; i < n->n_peers; i++, p++) {
	if (!strcmp(p->peername, peername))
	    return p;
    }
    return NULL;
}

static net_db_peer *
netdbPeerAdd(netdbEntry * n, peer * e)
{
    net_db_peer *p;
    net_db_peer *o;
    int osize;
    int i;
    if (n->n_peers == n->n_peers_alloc) {
	o = n->peers;
	osize = n->n_peers_alloc;
	if (n->n_peers_alloc == 0)
	    n->n_peers_alloc = 2;
	else
	    n->n_peers_alloc <<= 1;
	debug(38, 3) ("netdbPeerAdd: Growing peer list for '%s' to %d\n",
	    n->network, n->n_peers_alloc);
	n->peers = xcalloc(n->n_peers_alloc, sizeof(net_db_peer));
	for (i = 0; i < osize; i++)
	    *(n->peers + i) = *(o + i);
	if (osize) {
	    safe_free(o);
	}
    }
    p = n->peers + n->n_peers;
    p->peername = netdbPeerName(e->host);
    n->n_peers++;
    return p;
}

static int
sortPeerByRtt(const void *A, const void *B)
{
    const net_db_peer *p1 = A;
    const net_db_peer *p2 = B;
    if (p1->rtt > p2->rtt)
	return 1;
    else if (p1->rtt < p2->rtt)
	return -1;
    else
	return 0;
}

static void
netdbSaveState(void *foo)
{
    LOCAL_ARRAY(char, path, SQUID_MAXPATHLEN);
    Logfile *lf;
    netdbEntry *n;
    net_db_name *x;
    struct timeval start = current_time;
    int count = 0;
    snprintf(path, SQUID_MAXPATHLEN, "%s/netdb_state", storeSwapDir(0));
    /*
     * This was nicer when we were using stdio, but thanks to
     * Solaris bugs, its a bad idea.  fopen can fail if more than
     * 256 FDs are open.
     */
    /*
     * unlink() is here because there is currently no way to make
     * logfileOpen() use O_TRUNC.
     */
    unlink(path);
    lf = logfileOpen(path, 4096, 0);
    if (NULL == lf) {
	debug(50, 1) ("netdbSaveState: %s: %s\n", path, xstrerror());
	return;
    }
    hash_first(addr_table);
    while ((n = (netdbEntry *) hash_next(addr_table))) {
	if (n->pings_recv == 0)
	    continue;
	logfilePrintf(lf, "%s %d %d %10.5f %10.5f %d %d",
	    n->network,
	    n->pings_sent,
	    n->pings_recv,
	    n->hops,
	    n->rtt,
	    (int) n->next_ping_time,
	    (int) n->last_use_time);
	for (x = n->hosts; x; x = x->next)
	    logfilePrintf(lf, " %s", hashKeyStr(&x->hash));
	logfilePrintf(lf, "\n");
	count++;
#undef RBUF_SZ
    }
    logfileClose(lf);
    getCurrentTime();
    debug(38, 1) ("NETDB state saved; %d entries, %d msec\n",
	count, tvSubMsec(start, current_time));
    eventAddIsh("netdbSaveState", netdbSaveState, NULL, 3600.0, 1);
}

static void
netdbReloadState(void)
{
    LOCAL_ARRAY(char, path, SQUID_MAXPATHLEN);
    char *buf;
    char *t;
    char *s;
    int fd;
    int l;
    struct stat sb;
    netdbEntry *n;
    netdbEntry N;
    struct in_addr addr;
    int count = 0;
    struct timeval start = current_time;
    snprintf(path, SQUID_MAXPATHLEN, "%s/netdb_state", storeSwapDir(0));
    /*
     * This was nicer when we were using stdio, but thanks to
     * Solaris bugs, its a bad idea.  fopen can fail if more than
     * 256 FDs are open.
     */
    fd = file_open(path, O_RDONLY | O_TEXT);
    if (fd < 0)
	return;
    if (fstat(fd, &sb) < 0) {
	file_close(fd);
	return;
    }
    t = buf = xcalloc(1, sb.st_size + 1);
    l = FD_READ_METHOD(fd, buf, sb.st_size);
    file_close(fd);
    if (l <= 0)
	return;
    while ((s = strchr(t, '\n'))) {
	char *q;
	assert(s - buf < l);
	*s = '\0';
	memset(&N, '\0', sizeof(netdbEntry));
	q = strtok(t, w_space);
	t = s + 1;
	if (NULL == q)
	    continue;
	if (!safe_inet_addr(q, &addr))
	    continue;
	if (netdbLookupAddr(addr) != NULL)	/* no dups! */
	    continue;
	if ((q = strtok(NULL, w_space)) == NULL)
	    continue;
	N.pings_sent = atoi(q);
	if ((q = strtok(NULL, w_space)) == NULL)
	    continue;
	N.pings_recv = atoi(q);
	if (N.pings_recv == 0)
	    continue;
	/* give this measurement low weight */
	N.pings_sent = 1;
	N.pings_recv = 1;
	if ((q = strtok(NULL, w_space)) == NULL)
	    continue;
	N.hops = atof(q);
	if ((q = strtok(NULL, w_space)) == NULL)
	    continue;
	N.rtt = atof(q);
	if ((q = strtok(NULL, w_space)) == NULL)
	    continue;
	N.next_ping_time = (time_t) atoi(q);
	if ((q = strtok(NULL, w_space)) == NULL)
	    continue;
	N.last_use_time = (time_t) atoi(q);
	n = memAllocate(MEM_NETDBENTRY);
	xmemcpy(n, &N, sizeof(netdbEntry));
	netdbHashInsert(n, addr);
	while ((q = strtok(NULL, w_space)) != NULL) {
	    if (netdbLookupHost(q) != NULL)	/* no dups! */
		continue;
	    netdbHostInsert(n, q);
	}
	count++;
    }
    xfree(buf);
    getCurrentTime();
    debug(38, 1) ("NETDB state reloaded; %d entries, %d msec\n",
	count, tvSubMsec(start, current_time));
}

static const char *
netdbPeerName(const char *name)
{
    const wordlist *w;
    for (w = peer_names; w; w = w->next) {
	if (!strcmp(w->key, name))
	    return w->key;
    }
    return wordlistAdd(&peer_names, name);
}

static void
netdbFreeNetdbEntry(void *data)
{
    netdbEntry *n = data;
    safe_free(n->peers);
    memFree(n, MEM_NETDBENTRY);
}

static void
netdbFreeNameEntry(void *data)
{
    net_db_name *x = data;
    xfree(x->hash.key);
    memFree(x, MEM_NET_DB_NAME);
}

static void
netdbExchangeHandleReply(void *data, char *buf, ssize_t size)
{
    netdbExchangeState *ex = data;
    int rec_sz = 0;
    ssize_t o;
    struct in_addr addr;
    double rtt;
    double hops;
    char *p;
    int j;
    HttpReply *rep;
    size_t hdr_sz;
    int nused = 0;
    rec_sz = 0;
    rec_sz += 1 + sizeof(addr.s_addr);
    rec_sz += 1 + sizeof(int);
    rec_sz += 1 + sizeof(int);
    ex->seen = ex->used + size;
    debug(38, 3) ("netdbExchangeHandleReply: %d bytes\n", (int) size);
    if (!cbdataValid(ex->p)) {
	debug(38, 3) ("netdbExchangeHandleReply: Peer became invalid\n");
	netdbExchangeDone(ex);
	return;
    }
    debug(38, 3) ("netdbExchangeHandleReply: for '%s:%d'\n", ex->p->host, ex->p->http_port);
    p = buf;
    if (0 == ex->used) {
	/* skip reply headers */
	if ((hdr_sz = headersEnd(p, size))) {
	    debug(38, 5) ("netdbExchangeHandleReply: hdr_sz = %d\n", hdr_sz);
	    rep = ex->e->mem_obj->reply;
	    if (0 == rep->sline.status)
		httpReplyParse(rep, buf, hdr_sz);
	    debug(38, 3) ("netdbExchangeHandleReply: reply status %d\n",
		rep->sline.status);
	    if (HTTP_OK != rep->sline.status) {
		netdbExchangeDone(ex);
		return;
	    }
	    assert(size >= hdr_sz);
	    ex->used += hdr_sz;
	    size -= hdr_sz;
	    p += hdr_sz;
	} else {
	    size = 0;
	}
    }
    debug(38, 5) ("netdbExchangeHandleReply: start parsing loop, size = %d\n",
	size);
    while (size >= rec_sz) {
	debug(38, 5) ("netdbExchangeHandleReply: in parsing loop, size = %d\n",
	    size);
	addr.s_addr = any_addr.s_addr;
	hops = rtt = 0.0;
	for (o = 0; o < rec_sz;) {
	    switch ((int) *(p + o)) {
	    case NETDB_EX_NETWORK:
		o++;
		xmemcpy(&addr.s_addr, p + o, sizeof(addr.s_addr));
		o += sizeof(addr.s_addr);
		break;
	    case NETDB_EX_RTT:
		o++;
		xmemcpy(&j, p + o, sizeof(int));
		o += sizeof(int);
		rtt = (double) ntohl(j) / 1000.0;
		break;
	    case NETDB_EX_HOPS:
		o++;
		xmemcpy(&j, p + o, sizeof(int));
		o += sizeof(int);
		hops = (double) ntohl(j) / 1000.0;
		break;
	    default:
		debug(38, 1) ("netdbExchangeHandleReply: corrupt data, aborting\n");
		netdbExchangeDone(ex);
		return;
	    }
	}
	if (addr.s_addr != any_addr.s_addr && rtt > 0)
	    netdbExchangeUpdatePeer(addr, ex->p, rtt, hops);
	assert(o == rec_sz);
	ex->used += rec_sz;
	size -= rec_sz;
	p += rec_sz;
	/*
	 * This is a fairly cpu-intensive loop, break after adding
	 * just a few
	 */
	if (++nused == 20)
	    break;
    }
    debug(38, 3) ("netdbExchangeHandleReply: used %d entries, (x %d bytes) == %d bytes total\n",
	nused, rec_sz, nused * rec_sz);
    debug(38, 3) ("netdbExchangeHandleReply: seen %ld, used %ld\n", (long int) ex->seen, (long int) ex->used);
    if (EBIT_TEST(ex->e->flags, ENTRY_ABORTED)) {
	debug(38, 3) ("netdbExchangeHandleReply: ENTRY_ABORTED\n");
	netdbExchangeDone(ex);
    } else if (ex->e->store_status == STORE_PENDING) {
	debug(38, 3) ("netdbExchangeHandleReply: STORE_PENDING\n");
	storeClientCopy(ex->sc, ex->e, ex->seen, ex->used, ex->buf_sz,
	    ex->buf, netdbExchangeHandleReply, ex);
    } else if (ex->seen < ex->e->mem_obj->inmem_hi) {
	debug(38, 3) ("netdbExchangeHandleReply: ex->e->mem_obj->inmem_hi\n");
	storeClientCopy(ex->sc, ex->e, ex->seen, ex->used, ex->buf_sz,
	    ex->buf, netdbExchangeHandleReply, ex);
    } else {
	debug(38, 3) ("netdbExchangeHandleReply: Done\n");
	netdbExchangeDone(ex);
    }
}

static void
netdbExchangeDone(void *data)
{
    netdbExchangeState *ex = data;
    debug(38, 3) ("netdbExchangeDone: %s\n", storeUrl(ex->e));
    memFree(ex->buf, MEM_4K_BUF);
    requestUnlink(ex->r);
    storeUnregister(ex->sc, ex->e, ex);
    storeUnlockObject(ex->e);
    cbdataUnlock(ex->p);
    cbdataFree(ex);
}

#endif /* USE_ICMP */

/* PUBLIC FUNCTIONS */

void
netdbInit(void)
{
#if USE_ICMP
    int n;
    if (addr_table)
	return;
    n = hashPrime(Config.Netdb.high / 4);
    addr_table = hash_create((HASHCMP *) strcmp, n, hash_string);
    n = hashPrime(3 * Config.Netdb.high / 4);
    host_table = hash_create((HASHCMP *) strcmp, n, hash_string);
    eventAddIsh("netdbSaveState", netdbSaveState, NULL, 3600.0, 1);
    netdbReloadState();
    cachemgrRegister("netdb",
	"Network Measurement Database",
	netdbDump, 0, 1);
#endif
}

void
netdbPingSite(const char *hostname)
{
#if USE_ICMP
    netdbEntry *n;
    generic_cbdata *h;
    if ((n = netdbLookupHost(hostname)) != NULL)
	if (n->next_ping_time > squid_curtime)
	    return;
    h = cbdataAlloc(generic_cbdata);
    h->data = xstrdup(hostname);
    ipcache_nbgethostbyname(hostname, netdbSendPing, h);
#endif
}

void
netdbHandlePingReply(const struct sockaddr_in *from, int hops, int rtt)
{
#if USE_ICMP
    netdbEntry *n;
    int N;
    debug(38, 3) ("netdbHandlePingReply: from %s\n", inet_ntoa(from->sin_addr));
    if ((n = netdbLookupAddr(from->sin_addr)) == NULL)
	return;
    N = ++n->pings_recv;
    if (N > 5)
	N = 5;
    if (rtt < 1.0)
	rtt = 1.0;
    n->hops = ((n->hops * (N - 1)) + hops) / N;
    n->rtt = ((n->rtt * (N - 1)) + rtt) / N;
    debug(38, 3) ("netdbHandlePingReply: %s; rtt=%5.1f  hops=%4.1f\n",
	n->network,
	n->rtt,
	n->hops);
#endif
}

void
netdbFreeMemory(void)
{
#if USE_ICMP
    hashFreeItems(addr_table, netdbFreeNetdbEntry);
    hashFreeMemory(addr_table);
    addr_table = NULL;
    hashFreeItems(host_table, netdbFreeNameEntry);
    hashFreeMemory(host_table);
    host_table = NULL;
    wordlistDestroy(&peer_names);
    peer_names = NULL;
#endif
}

int
netdbHops(struct in_addr addr)
{
#if USE_ICMP
    netdbEntry *n = netdbLookupAddr(addr);
    if (n && n->pings_recv) {
	n->last_use_time = squid_curtime;
	return (int) (n->hops + 0.5);
    }
#endif
    return 256;
}

void
netdbDump(StoreEntry * sentry)
{
#if USE_ICMP
    netdbEntry *n;
    netdbEntry **list;
    net_db_name *x;
    int k;
    int i;
    int j;
    net_db_peer *p;
    storeAppendPrintf(sentry, "Network DB Statistics:\n");
    storeAppendPrintf(sentry, "%-16.16s %9s %7s %5s %s\n",
	"Network",
	"recv/sent",
	"RTT",
	"Hops",
	"Hostnames");
    list = xcalloc(memInUse(MEM_NETDBENTRY), sizeof(netdbEntry *));
    i = 0;
    hash_first(addr_table);
    while ((n = (netdbEntry *) hash_next(addr_table)))
	*(list + i++) = n;
    if (i != memInUse(MEM_NETDBENTRY))
	debug(38, 0) ("WARNING: netdb_addrs count off, found %d, expected %d\n",
	    i, memInUse(MEM_NETDBENTRY));
    qsort((char *) list,
	i,
	sizeof(netdbEntry *),
	sortByRtt);
    for (k = 0; k < i; k++) {
	n = *(list + k);
	storeAppendPrintf(sentry, "%-16.16s %4d/%4d %7.1f %5.1f",
	    n->network,
	    n->pings_recv,
	    n->pings_sent,
	    n->rtt,
	    n->hops);
	for (x = n->hosts; x; x = x->next)
	    storeAppendPrintf(sentry, " %s", hashKeyStr(&x->hash));
	storeAppendPrintf(sentry, "\n");
	p = n->peers;
	for (j = 0; j < n->n_peers; j++, p++) {
	    storeAppendPrintf(sentry, "    %-22.22s %7.1f %5.1f\n",
		p->peername,
		p->rtt,
		p->hops);
	}
    }
    xfree(list);
#else
    http_reply *reply = sentry->mem_obj->reply;
    http_version_t version;
    httpReplyReset(reply);
    httpBuildVersion(&version, 1, 0);
    httpReplySetHeaders(reply, version, HTTP_BAD_REQUEST, "Bad Request",
	NULL, -1, squid_curtime, -2);
    httpReplySwapOut(reply, sentry);
    storeAppendPrintf(sentry,
	"NETDB support not compiled into this Squid cache.\n");
#endif
}

int
netdbHostHops(const char *host)
{
#if USE_ICMP
    netdbEntry *n = netdbLookupHost(host);
    if (n) {
	n->last_use_time = squid_curtime;
	return (int) (n->hops + 0.5);
    }
#endif
    return 0;
}

int
netdbHostRtt(const char *host)
{
#if USE_ICMP
    netdbEntry *n = netdbLookupHost(host);
    if (n) {
	n->last_use_time = squid_curtime;
	return (int) (n->rtt + 0.5);
    }
#endif
    return 0;
}

void
netdbHostData(const char *host, int *samp, int *rtt, int *hops)
{
#if USE_ICMP
    netdbEntry *n = netdbLookupHost(host);
    if (n == NULL)
	return;
    *samp = n->pings_recv;
    *rtt = (int) (n->rtt + 0.5);
    *hops = (int) (n->hops + 0.5);
    n->last_use_time = squid_curtime;
#endif
}

void
netdbUpdatePeer(request_t * r, peer * e, int irtt, int ihops)
{
#if USE_ICMP
    netdbEntry *n;
    double rtt = (double) irtt;
    double hops = (double) ihops;
    net_db_peer *p;
    debug(38, 3) ("netdbUpdatePeer: '%s', %d hops, %d rtt\n", r->host, ihops, irtt);
    n = netdbLookupHost(r->host);
    if (n == NULL) {
	debug(38, 3) ("netdbUpdatePeer: host '%s' not found\n", r->host);
	return;
    }
    if ((p = netdbPeerByName(n, e->host)) == NULL)
	p = netdbPeerAdd(n, e);
    p->rtt = rtt;
    p->hops = hops;
    p->expires = squid_curtime + 3600;
    if (n->n_peers < 2)
	return;
    qsort((char *) n->peers,
	n->n_peers,
	sizeof(net_db_peer),
	sortPeerByRtt);
#endif
}

void
netdbExchangeUpdatePeer(struct in_addr addr, peer * e, double rtt, double hops)
{
#if USE_ICMP
    netdbEntry *n;
    net_db_peer *p;
    debug(38, 5) ("netdbExchangeUpdatePeer: '%s', %0.1f hops, %0.1f rtt\n",
	inet_ntoa(addr), hops, rtt);
    n = netdbLookupAddr(addr);
    if (n == NULL)
	n = netdbAdd(addr);
    assert(NULL != n);
    if ((p = netdbPeerByName(n, e->host)) == NULL)
	p = netdbPeerAdd(n, e);
    p->rtt = rtt;
    p->hops = hops;
    p->expires = squid_curtime + 3600;	/* XXX ? */
    if (n->n_peers < 2)
	return;
    qsort((char *) n->peers,
	n->n_peers,
	sizeof(net_db_peer),
	sortPeerByRtt);
#endif
}

void
netdbDeleteAddrNetwork(struct in_addr addr)
{
#if USE_ICMP
    netdbEntry *n = netdbLookupAddr(addr);
    if (n == NULL)
	return;
    debug(38, 3) ("netdbDeleteAddrNetwork: %s\n", n->network);
    netdbRelease(n);
#endif
}

void
netdbBinaryExchange(StoreEntry * s)
{
    http_reply *reply = s->mem_obj->reply;
    http_version_t version;
#if USE_ICMP
    netdbEntry *n;
    int i;
    int j;
    int rec_sz;
    char *buf;
    struct in_addr addr;
    storeBuffer(s);
    httpReplyReset(reply);
    httpBuildVersion(&version, 1, 0);
    httpReplySetHeaders(reply, version, HTTP_OK, "OK",
	NULL, -1, squid_curtime, -2);
    httpReplySwapOut(reply, s);
    rec_sz = 0;
    rec_sz += 1 + sizeof(addr.s_addr);
    rec_sz += 1 + sizeof(int);
    rec_sz += 1 + sizeof(int);
    buf = memAllocate(MEM_4K_BUF);
    i = 0;
    hash_first(addr_table);
    while ((n = (netdbEntry *) hash_next(addr_table))) {
	if (0.0 == n->rtt)
	    continue;
	if (n->rtt > 60000)	/* RTT > 1 MIN probably bogus */
	    continue;
	if (!safe_inet_addr(n->network, &addr))
	    continue;
	buf[i++] = (char) NETDB_EX_NETWORK;
	xmemcpy(&buf[i], &addr.s_addr, sizeof(addr.s_addr));
	i += sizeof(addr.s_addr);
	buf[i++] = (char) NETDB_EX_RTT;
	j = htonl((int) (n->rtt * 1000));
	xmemcpy(&buf[i], &j, sizeof(int));
	i += sizeof(int);
	buf[i++] = (char) NETDB_EX_HOPS;
	j = htonl((int) (n->hops * 1000));
	xmemcpy(&buf[i], &j, sizeof(int));
	i += sizeof(int);
	if (i + rec_sz > 4096) {
	    storeAppend(s, buf, i);
	    i = 0;
	}
    }
    if (i > 0) {
	storeAppend(s, buf, i);
	i = 0;
    }
    assert(0 == i);
    storeBufferFlush(s);
    memFree(buf, MEM_4K_BUF);
#else
    httpReplyReset(reply);
    httpBuildVersion(&version, 1, 0);
    httpReplySetHeaders(reply, version, HTTP_BAD_REQUEST, "Bad Request",
	NULL, -1, squid_curtime, -2);
    httpReplySwapOut(reply, s);
    storeAppendPrintf(s, "NETDB support not compiled into this Squid cache.\n");
#endif
    storeComplete(s);
}

#if USE_ICMP
CBDATA_TYPE(netdbExchangeState);
#endif

void
netdbExchangeStart(void *data)
{
#if USE_ICMP
    peer *p = data;
    char *uri;
    netdbExchangeState *ex;
    CBDATA_INIT_TYPE(netdbExchangeState);
    ex = cbdataAlloc(netdbExchangeState);
    cbdataLock(p);
    ex->p = p;
    uri = internalRemoteUri(p->host, p->http_port, "/squid-internal-dynamic/", "netdb");
    debug(38, 3) ("netdbExchangeStart: Requesting '%s'\n", uri);
    assert(NULL != uri);
    ex->r = urlParse(METHOD_GET, uri);
    if (NULL == ex->r) {
	debug(38, 1) ("netdbExchangeStart: Bad URI %s\n", uri);
	return;
    }
    requestLink(ex->r);
    assert(NULL != ex->r);
    httpBuildVersion(&ex->r->http_ver, 1, 0);
    ex->e = storeCreateEntry(uri, uri, null_request_flags, METHOD_GET);
    ex->buf_sz = 4096;
    ex->buf = memAllocate(MEM_4K_BUF);
    assert(NULL != ex->e);
    ex->sc = storeClientListAdd(ex->e, ex);
    storeClientCopy(ex->sc, ex->e, ex->seen, ex->used, ex->buf_sz,
	ex->buf, netdbExchangeHandleReply, ex);
    ex->r->flags.loopdetect = 1;	/* cheat! -- force direct */
    if (p->login)
	xstrncpy(ex->r->login, p->login, MAX_LOGIN_SZ);
    fwdStart(-1, ex->e, ex->r);
#endif
}

peer *
netdbClosestParent(request_t * request)
{
#if USE_ICMP
    peer *p = NULL;
    netdbEntry *n;
    const ipcache_addrs *ia;
    net_db_peer *h;
    int i;
    n = netdbLookupHost(request->host);
    if (NULL == n) {
	/* try IP addr */
	ia = ipcache_gethostbyname(request->host, 0);
	if (NULL != ia)
	    n = netdbLookupAddr(ia->in_addrs[ia->cur]);
    }
    if (NULL == n)
	return NULL;
    if (0 == n->n_peers)
	return NULL;
    n->last_use_time = squid_curtime;
    /* 
     * Find the parent with the least RTT to the origin server.
     * Make sure we don't return a parent who is farther away than
     * we are.  Note, the n->peers list is pre-sorted by RTT.
     */
    for (i = 0; i < n->n_peers; i++) {
	h = &n->peers[i];
	if (n->rtt > 0)
	    if (n->rtt < h->rtt)
		break;
	p = peerFindByName(h->peername);
	if (NULL == p)		/* not found */
	    continue;
	if (neighborType(p, request) != PEER_PARENT)
	    continue;
	if (!peerHTTPOkay(p, request))	/* not allowed */
	    continue;
	return p;
    }
#endif
    return NULL;
}
