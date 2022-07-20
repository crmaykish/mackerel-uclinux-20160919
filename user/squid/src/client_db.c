
/*
 * $Id: client_db.c,v 1.53.2.5 2005/03/26 02:50:52 hno Exp $
 *
 * DEBUG: section 0     Client Database
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

static hash_table *client_table = NULL;
static ClientInfo *clientdbAdd(struct in_addr addr);
static FREE clientdbFreeItem;
static void clientdbStartGC(void);
static void clientdbScheduledGC(void *);

static int max_clients = 32;
static int cleanup_running = 0;
static int cleanup_scheduled = 0;
static int cleanup_removed;

#define CLIENT_DB_HASH_SIZE 467

static ClientInfo *
clientdbAdd(struct in_addr addr)
{
    ClientInfo *c;
    c = memAllocate(MEM_CLIENT_INFO);
    c->hash.key = xstrdup(inet_ntoa(addr));
    c->addr = addr;
    hash_join(client_table, &c->hash);
    statCounter.client_http.clients++;
    if ((statCounter.client_http.clients > max_clients) && !cleanup_running && cleanup_scheduled < 2) {
	cleanup_scheduled++;
	eventAdd("client_db garbage collector", clientdbScheduledGC, NULL, 90, 0);
    } 
    return c;
}

void
clientdbInit(void)
{
    if (client_table)
	return;
    client_table = hash_create((HASHCMP *) strcmp, CLIENT_DB_HASH_SIZE, hash_string);
    cachemgrRegister("client_list",
	"Cache Client List",
	clientdbDump,
	0, 1);
}

void
clientdbUpdate(struct in_addr addr, log_type ltype, protocol_t p, squid_off_t size)
{
    char *key;
    ClientInfo *c;
    if (!Config.onoff.client_db)
	return;
    key = inet_ntoa(addr);
    c = (ClientInfo *) hash_lookup(client_table, key);
    if (c == NULL)
	c = clientdbAdd(addr);
    if (c == NULL)
	debug_trap("clientdbUpdate: Failed to add entry");
    if (p == PROTO_HTTP) {
	c->Http.n_requests++;
	c->Http.result_hist[ltype]++;
	kb_incr(&c->Http.kbytes_out, size);
	if (isTcpHit(ltype))
	    kb_incr(&c->Http.hit_kbytes_out, size);
    } else if (p == PROTO_ICP) {
	c->Icp.n_requests++;
	c->Icp.result_hist[ltype]++;
	kb_incr(&c->Icp.kbytes_out, size);
	if (LOG_UDP_HIT == ltype)
	    kb_incr(&c->Icp.hit_kbytes_out, size);
    }
    c->last_seen = squid_curtime;
}

/*
 * clientdbEstablished()
 * This function tracks the number of currently established connections
 * for a client IP address.  When a connection is accepted, call this
 * with delta = 1.  When the connection is closed, call with delta =
 * -1.  To get the current value, simply call with delta = 0.
 */
int
clientdbEstablished(struct in_addr addr, int delta)
{
    char *key;
    ClientInfo *c;
    if (!Config.onoff.client_db)
	return 0;
    key = inet_ntoa(addr);
    c = (ClientInfo *) hash_lookup(client_table, key);
    if (c == NULL)
	c = clientdbAdd(addr);
    if (c == NULL)
	debug_trap("clientdbUpdate: Failed to add entry");
    c->n_established += delta;
    return c->n_established;
}

#define CUTOFF_SECONDS 3600
int
clientdbCutoffDenied(struct in_addr addr)
{
    char *key;
    int NR;
    int ND;
    double p;
    ClientInfo *c;
    if (!Config.onoff.client_db)
	return 0;
    key = inet_ntoa(addr);
    c = (ClientInfo *) hash_lookup(client_table, key);
    if (c == NULL)
	return 0;
    /*
     * If we are in a cutoff window, we don't send a reply
     */
    if (squid_curtime - c->cutoff.time < CUTOFF_SECONDS)
	return 1;
    /*
     * Calculate the percent of DENIED replies since the last
     * cutoff time.
     */
    NR = c->Icp.n_requests - c->cutoff.n_req;
    if (NR < 150)
	NR = 150;
    ND = c->Icp.result_hist[LOG_UDP_DENIED] - c->cutoff.n_denied;
    p = 100.0 * ND / NR;
    if (p < 95.0)
	return 0;
    debug(1, 0) ("WARNING: Probable misconfigured neighbor at %s\n", key);
    debug(1, 0) ("WARNING: %d of the last %d ICP replies are DENIED\n", ND, NR);
    debug(1, 0) ("WARNING: No replies will be sent for the next %d seconds\n",
	CUTOFF_SECONDS);
    c->cutoff.time = squid_curtime;
    c->cutoff.n_req = c->Icp.n_requests;
    c->cutoff.n_denied = c->Icp.result_hist[LOG_UDP_DENIED];
    return 1;
}


void
clientdbDump(StoreEntry * sentry)
{
    ClientInfo *c;
    log_type l;
    int icp_total = 0;
    int icp_hits = 0;
    int http_total = 0;
    int http_hits = 0;
    storeAppendPrintf(sentry, "Cache Clients:\n");
    hash_first(client_table);
    while ((c = (ClientInfo *) hash_next(client_table))) {
	storeAppendPrintf(sentry, "Address: %s\n", hashKeyStr(&c->hash));
	storeAppendPrintf(sentry, "Name: %s\n", fqdnFromAddr(c->addr));
	storeAppendPrintf(sentry, "Currently established connections: %d\n",
	    c->n_established);
	storeAppendPrintf(sentry, "    ICP Requests %d\n",
	    c->Icp.n_requests);
	for (l = LOG_TAG_NONE; l < LOG_TYPE_MAX; l++) {
	    if (c->Icp.result_hist[l] == 0)
		continue;
	    icp_total += c->Icp.result_hist[l];
	    if (LOG_UDP_HIT == l)
		icp_hits += c->Icp.result_hist[l];
	    storeAppendPrintf(sentry,
		"        %-20.20s %7d %3d%%\n",
		log_tags[l],
		c->Icp.result_hist[l],
		percent(c->Icp.result_hist[l], c->Icp.n_requests));
	}
	storeAppendPrintf(sentry, "    HTTP Requests %d\n",
	    c->Http.n_requests);
	for (l = LOG_TAG_NONE; l < LOG_TYPE_MAX; l++) {
	    if (c->Http.result_hist[l] == 0)
		continue;
	    http_total += c->Http.result_hist[l];
	    if (isTcpHit(l))
		http_hits += c->Http.result_hist[l];
	    storeAppendPrintf(sentry,
		"        %-20.20s %7d %3d%%\n",
		log_tags[l],
		c->Http.result_hist[l],
		percent(c->Http.result_hist[l], c->Http.n_requests));
	}
	storeAppendPrintf(sentry, "\n");
    }
    storeAppendPrintf(sentry, "TOTALS\n");
    storeAppendPrintf(sentry, "ICP : %d Queries, %d Hits (%3d%%)\n",
	icp_total, icp_hits, percent(icp_hits, icp_total));
    storeAppendPrintf(sentry, "HTTP: %d Requests, %d Hits (%3d%%)\n",
	http_total, http_hits, percent(http_hits, http_total));
}

static void
clientdbFreeItem(void *data)
{
    ClientInfo *c = data;
    safe_free(c->hash.key);
    memFree(c, MEM_CLIENT_INFO);
}

void
clientdbFreeMemory(void)
{
    hashFreeItems(client_table, clientdbFreeItem);
    hashFreeMemory(client_table);
    client_table = NULL;
}

static void
clientdbScheduledGC(void *unused)
{
    cleanup_scheduled = 0;
    clientdbStartGC();
}

static void
clientdbGC(void *unused)
{
    static int bucket = 0;
    hash_link *link_next;

    link_next = hash_get_bucket(client_table, bucket++);
    while (link_next != NULL) {
	ClientInfo *c = (ClientInfo *) link_next;
	int age = squid_curtime - c->last_seen;
	link_next = link_next->next;
	if (c->n_established)
	    continue;

	if (age < 24 * 3600 && c->Http.n_requests > 100)
	    continue;
	if (age < 4 * 3600 && (c->Http.n_requests > 10 || c->Icp.n_requests > 10))
	    continue;
	if (age < 5 * 60 && (c->Http.n_requests > 1 || c->Icp.n_requests > 1))
	    continue;
	if (age < 60)
	    continue;
	hash_remove_link(client_table, &c->hash);
	clientdbFreeItem(c);
	statCounter.client_http.clients--;
	cleanup_removed++;
    }

    if (bucket < CLIENT_DB_HASH_SIZE)
	eventAdd("client_db garbage collector", clientdbGC, NULL, 0.15, 0);
    else {
	bucket = 0;
	cleanup_running = 0;
	max_clients = statCounter.client_http.clients * 3 / 2;
	if (!cleanup_scheduled) {
	    cleanup_scheduled = 1;
	    eventAdd("client_db garbage collector", clientdbScheduledGC, NULL, 3 * 3600, 0);
	}
	debug(49, 2) ("clientdbGC: Removed %d entries\n", cleanup_removed);
    }
}

static void
clientdbStartGC(void)
{
    max_clients = statCounter.client_http.clients;
    cleanup_running = 1;
    cleanup_removed = 0;
    clientdbGC(NULL);
}

#if SQUID_SNMP
struct in_addr *
client_entry(struct in_addr *current)
{
    ClientInfo *c = NULL;
    char *key;
    if (current) {
	key = inet_ntoa(*current);
	hash_first(client_table);
	while ((c = (ClientInfo *) hash_next(client_table))) {
	    if (!strcmp(key, hashKeyStr(&c->hash)))
		break;
	}
	c = (ClientInfo *) hash_next(client_table);
    } else {
	hash_first(client_table);
	c = (ClientInfo *) hash_next(client_table);
    }
    hash_last(client_table);
    if (c)
	return (&c->addr);
    else
	return (NULL);

}

variable_list *
snmp_meshCtblFn(variable_list * Var, snint * ErrP)
{
    variable_list *Answer = NULL;
    static char key[15];
    ClientInfo *c = NULL;
    int aggr = 0;
    log_type l;
    *ErrP = SNMP_ERR_NOERROR;
    debug(49, 6) ("snmp_meshCtblFn: Current : \n");
    snmpDebugOid(6, Var->name, Var->name_length);
    snprintf(key, sizeof(key), "%d.%d.%d.%d", Var->name[LEN_SQ_NET + 3], Var->name[LEN_SQ_NET + 4],
	Var->name[LEN_SQ_NET + 5], Var->name[LEN_SQ_NET + 6]);
    debug(49, 5) ("snmp_meshCtblFn: [%s] requested!\n", key);
    c = (ClientInfo *) hash_lookup(client_table, key);
    if (c == NULL) {
	debug(49, 5) ("snmp_meshCtblFn: not found.\n");
	*ErrP = SNMP_ERR_NOSUCHNAME;
	return NULL;
    }
    switch (Var->name[LEN_SQ_NET + 2]) {
    case MESH_CTBL_ADDR:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->addr.s_addr,
	    SMI_IPADDRESS);
	break;
    case MESH_CTBL_HTBYTES:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->Http.kbytes_out.kb,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_HTREQ:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->Http.n_requests,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_HTHITS:
	aggr = 0;
	for (l = LOG_TAG_NONE; l < LOG_TYPE_MAX; l++) {
	    if (isTcpHit(l))
		aggr += c->Http.result_hist[l];
	}
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) aggr,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_HTHITBYTES:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->Http.hit_kbytes_out.kb,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_ICPBYTES:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->Icp.kbytes_out.kb,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_ICPREQ:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->Icp.n_requests,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_ICPHITS:
	aggr = c->Icp.result_hist[LOG_UDP_HIT];
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) aggr,
	    SMI_COUNTER32);
	break;
    case MESH_CTBL_ICPHITBYTES:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    (snint) c->Icp.hit_kbytes_out.kb,
	    SMI_COUNTER32);
	break;
    default:
	*ErrP = SNMP_ERR_NOSUCHNAME;
	debug(49, 5) ("snmp_meshCtblFn: illegal column.\n");
	break;
    }
    return Answer;
}

#endif /*SQUID_SNMP */
