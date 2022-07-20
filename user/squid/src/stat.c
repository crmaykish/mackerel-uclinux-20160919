
/*
 * $Id: stat.c,v 1.351.2.16 2005/03/29 09:52:00 hno Exp $
 *
 * DEBUG: section 18    Cache Manager Statistics
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

typedef int STOBJFLT(const StoreEntry *);
typedef struct {
    StoreEntry *sentry;
    int bucket;
    STOBJFLT *filter;
} StatObjectsState;

/* LOCALS */
static const char *describeStatuses(const StoreEntry *);
static const char *describeTimestamps(const StoreEntry *);
static void statAvgTick(void *notused);
static void statAvgDump(StoreEntry *, int minutes, int hours);
#if STAT_GRAPHS
static void statGraphDump(StoreEntry *);
#endif
static void statCountersInit(StatCounters *);
static void statCountersInitSpecial(StatCounters *);
static void statCountersClean(StatCounters *);
static void statCountersCopy(StatCounters * dest, const StatCounters * orig);
static double statMedianSvc(int, int);
static void statStoreEntry(MemBuf * mb, StoreEntry * e);
static double statCPUUsage(int minutes);
static OBJH stat_io_get;
static OBJH stat_objects_get;
static OBJH stat_vmobjects_get;
static OBJH statOpenfdObj;
static EVH statObjects;
static OBJH info_get;
static OBJH statFiledescriptors;
static OBJH statCountersDump;
static OBJH statPeerSelect;
static OBJH statDigestBlob;
static OBJH statAvg5min;
static OBJH statAvg60min;
static OBJH statUtilization;
static OBJH statCountersHistograms;
static OBJH statClientRequests;

#ifdef XMALLOC_STATISTICS
static void info_get_mallstat(int, int, int, void *);
static double xm_time;
static double xm_deltat;
#endif

StatCounters CountHist[N_COUNT_HIST];
static int NCountHist = 0;
static StatCounters CountHourHist[N_COUNT_HOUR_HIST];
static int NCountHourHist = 0;
CBDATA_TYPE(StatObjectsState);

extern unsigned int mem_pool_alloc_calls;
extern unsigned int mem_pool_free_calls;

static void
statUtilization(StoreEntry * e)
{
    storeAppendPrintf(e, "Cache Utilisation:\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Last 5 minutes:\n");
    if (NCountHist >= 5)
	statAvgDump(e, 5, 0);
    else
	storeAppendPrintf(e, "(no values recorded yet)\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Last 15 minutes:\n");
    if (NCountHist >= 15)
	statAvgDump(e, 15, 0);
    else
	storeAppendPrintf(e, "(no values recorded yet)\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Last hour:\n");
    if (NCountHist >= 60)
	statAvgDump(e, 60, 0);
    else
	storeAppendPrintf(e, "(no values recorded yet)\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Last 8 hours:\n");
    if (NCountHourHist >= 8)
	statAvgDump(e, 0, 8);
    else
	storeAppendPrintf(e, "(no values recorded yet)\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Last day:\n");
    if (NCountHourHist >= 24)
	statAvgDump(e, 0, 24);
    else
	storeAppendPrintf(e, "(no values recorded yet)\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Last 3 days:\n");
    if (NCountHourHist >= 72)
	statAvgDump(e, 0, 72);
    else
	storeAppendPrintf(e, "(no values recorded yet)\n");
    storeAppendPrintf(e, "\n");
    storeAppendPrintf(e, "Totals since cache startup:\n");
    statCountersDump(e);
}

static void
stat_io_get(StoreEntry * sentry)
{
    int i;

    storeAppendPrintf(sentry, "HTTP I/O\n");
    storeAppendPrintf(sentry, "number of reads: %d\n", IOStats.Http.reads);
    storeAppendPrintf(sentry, "Read Histogram:\n");
    for (i = 0; i < 16; i++) {
	storeAppendPrintf(sentry, "%5d-%5d: %9d %2d%%\n",
	    i ? (1 << (i - 1)) + 1 : 1,
	    1 << i,
	    IOStats.Http.read_hist[i],
	    percent(IOStats.Http.read_hist[i], IOStats.Http.reads));
    }

    storeAppendPrintf(sentry, "\n");
    storeAppendPrintf(sentry, "FTP I/O\n");
    storeAppendPrintf(sentry, "number of reads: %d\n", IOStats.Ftp.reads);
    storeAppendPrintf(sentry, "Read Histogram:\n");
    for (i = 0; i < 16; i++) {
	storeAppendPrintf(sentry, "%5d-%5d: %9d %2d%%\n",
	    i ? (1 << (i - 1)) + 1 : 1,
	    1 << i,
	    IOStats.Ftp.read_hist[i],
	    percent(IOStats.Ftp.read_hist[i], IOStats.Ftp.reads));
    }

    storeAppendPrintf(sentry, "\n");
    storeAppendPrintf(sentry, "Gopher I/O\n");
    storeAppendPrintf(sentry, "number of reads: %d\n", IOStats.Gopher.reads);
    storeAppendPrintf(sentry, "Read Histogram:\n");
    for (i = 0; i < 16; i++) {
	storeAppendPrintf(sentry, "%5d-%5d: %9d %2d%%\n",
	    i ? (1 << (i - 1)) + 1 : 1,
	    1 << i,
	    IOStats.Gopher.read_hist[i],
	    percent(IOStats.Gopher.read_hist[i], IOStats.Gopher.reads));
    }

    storeAppendPrintf(sentry, "\n");
    storeAppendPrintf(sentry, "WAIS I/O\n");
    storeAppendPrintf(sentry, "number of reads: %d\n", IOStats.Wais.reads);
    storeAppendPrintf(sentry, "Read Histogram:\n");
    for (i = 0; i < 16; i++) {
	storeAppendPrintf(sentry, "%5d-%5d: %9d %2d%%\n",
	    i ? (1 << (i - 1)) + 1 : 1,
	    1 << i,
	    IOStats.Wais.read_hist[i],
	    percent(IOStats.Wais.read_hist[i], IOStats.Wais.reads));
    }
}

static const char *
describeStatuses(const StoreEntry * entry)
{
    LOCAL_ARRAY(char, buf, 256);
    snprintf(buf, 256, "%-13s %-13s %-12s %-12s",
	storeStatusStr[entry->store_status],
	memStatusStr[entry->mem_status],
	swapStatusStr[entry->swap_status],
	pingStatusStr[entry->ping_status]);
    return buf;
}

const char *
storeEntryFlags(const StoreEntry * entry)
{
    LOCAL_ARRAY(char, buf, 256);
    int flags = (int) entry->flags;
    char *t;
    buf[0] = '\0';
    if (EBIT_TEST(flags, ENTRY_SPECIAL))
	strcat(buf, "SPECIAL,");
    if (EBIT_TEST(flags, ENTRY_REVALIDATE))
	strcat(buf, "REVALIDATE,");
    if (EBIT_TEST(flags, DELAY_SENDING))
	strcat(buf, "DELAY_SENDING,");
    if (EBIT_TEST(flags, RELEASE_REQUEST))
	strcat(buf, "RELEASE_REQUEST,");
    if (EBIT_TEST(flags, REFRESH_REQUEST))
	strcat(buf, "REFRESH_REQUEST,");
    if (EBIT_TEST(flags, ENTRY_CACHABLE))
	strcat(buf, "CACHABLE,");
    if (EBIT_TEST(flags, ENTRY_DISPATCHED))
	strcat(buf, "DISPATCHED,");
    if (EBIT_TEST(flags, KEY_PRIVATE))
	strcat(buf, "PRIVATE,");
    if (EBIT_TEST(flags, ENTRY_FWD_HDR_WAIT))
	strcat(buf, "FWD_HDR_WAIT,");
    if (EBIT_TEST(flags, ENTRY_NEGCACHED))
	strcat(buf, "NEGCACHED,");
    if (EBIT_TEST(flags, ENTRY_VALIDATED))
	strcat(buf, "VALIDATED,");
    if (EBIT_TEST(flags, ENTRY_BAD_LENGTH))
	strcat(buf, "BAD_LENGTH,");
    if (EBIT_TEST(flags, ENTRY_ABORTED))
	strcat(buf, "ABORTED,");
    if ((t = strrchr(buf, ',')))
	*t = '\0';
    return buf;
}

static const char *
describeTimestamps(const StoreEntry * entry)
{
    LOCAL_ARRAY(char, buf, 256);
    snprintf(buf, 256, "LV:%-9d LU:%-9d LM:%-9d EX:%-9d",
	(int) entry->timestamp,
	(int) entry->lastref,
	(int) entry->lastmod,
	(int) entry->expires);
    return buf;
}

static void
statStoreEntry(MemBuf * mb, StoreEntry * e)
{
    MemObject *mem = e->mem_obj;
    int i;
    struct _store_client *sc;
    dlink_node *node;
    memBufPrintf(mb, "KEY %s\n", storeKeyText(e->hash.key));
    if (mem)
	memBufPrintf(mb, "\t%s %s\n",
	    RequestMethodStr[mem->method], mem->log_url);
    memBufPrintf(mb, "\t%s\n", describeStatuses(e));
    memBufPrintf(mb, "\t%s\n", storeEntryFlags(e));
    memBufPrintf(mb, "\t%s\n", describeTimestamps(e));
    memBufPrintf(mb, "\t%d locks, %d clients, %d refs\n",
	(int) e->lock_count,
	storePendingNClients(e),
	(int) e->refcount);
    memBufPrintf(mb, "\tSwap Dir %d, File %#08X\n",
	e->swap_dirn, e->swap_filen);
    if (mem != NULL) {
	memBufPrintf(mb, "\tinmem_lo: %" PRINTF_OFF_T "\n", mem->inmem_lo);
	memBufPrintf(mb, "\tinmem_hi: %" PRINTF_OFF_T "\n", mem->inmem_hi);
	memBufPrintf(mb, "\tswapout: %" PRINTF_OFF_T " bytes queued\n",
	    mem->swapout.queue_offset);
	if (mem->swapout.sio)
	    memBufPrintf(mb, "\tswapout: %" PRINTF_OFF_T " bytes written\n",
		storeOffset(mem->swapout.sio));
	for (i = 0, node = mem->clients.head; node; node = node->next, i++) {
	    sc = (store_client *) node->data;
	    if (sc->callback_data == NULL)
		continue;
	    memBufPrintf(mb, "\tClient #%d, %p\n", i, sc->callback_data);
	    memBufPrintf(mb, "\t\tcopy_offset: %" PRINTF_OFF_T "\n",
		sc->copy_offset);
	    memBufPrintf(mb, "\t\tseen_offset: %" PRINTF_OFF_T "\n",
		sc->seen_offset);
	    memBufPrintf(mb, "\t\tcopy_size: %d\n",
		(int) sc->copy_size);
	    memBufPrintf(mb, "\t\tflags:");
	    if (sc->flags.disk_io_pending)
		memBufPrintf(mb, " disk_io_pending");
	    if (sc->flags.store_copying)
		memBufPrintf(mb, " store_copying");
	    if (sc->flags.copy_event_pending)
		memBufPrintf(mb, " copy_event_pending");
	    memBufPrintf(mb, "\n");
	}
    }
    memBufPrintf(mb, "\n");
}

/* process objects list */
static void
statObjects(void *data)
{
    StatObjectsState *state = data;
    StoreEntry *e;
    hash_link *link_ptr = NULL;
    hash_link *link_next = NULL;
    if (state->bucket >= store_hash_buckets) {
	storeComplete(state->sentry);
	storeUnlockObject(state->sentry);
	cbdataFree(state);
	return;
    } else if (EBIT_TEST(state->sentry->flags, ENTRY_ABORTED)) {
	storeUnlockObject(state->sentry);
	cbdataFree(state);
	return;
    } else if (fwdCheckDeferRead(-1, state->sentry)) {
	eventAdd("statObjects", statObjects, state, 0.1, 1);
	return;
    }
    debug(49, 3) ("statObjects: Bucket #%d\n", state->bucket);
    link_next = hash_get_bucket(store_table, state->bucket);
    if (link_next) {
	MemBuf mb;
	memBufDefInit(&mb);
	while (NULL != (link_ptr = link_next)) {
	    link_next = link_ptr->next;
	    e = (StoreEntry *) link_ptr;
	    if (state->filter && 0 == state->filter(e))
		continue;
	    statStoreEntry(&mb, e);
	}
	storeAppend(state->sentry, mb.buf, mb.size);
	memBufClean(&mb);
    }
    state->bucket++;
    eventAdd("statObjects", statObjects, state, 0.0, 1);
}

static void
statObjectsStart(StoreEntry * sentry, STOBJFLT * filter)
{
    StatObjectsState *state;
    state = cbdataAlloc(StatObjectsState);
    state->sentry = sentry;
    state->filter = filter;
    storeLockObject(sentry);
    eventAdd("statObjects", statObjects, state, 0.0, 1);
}

static void
stat_objects_get(StoreEntry * sentry)
{
    statObjectsStart(sentry, NULL);
}

static int
statObjectsVmFilter(const StoreEntry * e)
{
    return e->mem_obj ? 1 : 0;
}

static void
stat_vmobjects_get(StoreEntry * sentry)
{
    statObjectsStart(sentry, statObjectsVmFilter);
}

static int
statObjectsOpenfdFilter(const StoreEntry * e)
{
    if (e->mem_obj == NULL)
	return 0;
    if (e->mem_obj->swapout.sio == NULL)
	return 0;
    return 1;
}

static void
statOpenfdObj(StoreEntry * sentry)
{
    statObjectsStart(sentry, statObjectsOpenfdFilter);
}

static int
statObjectsPendingFilter(const StoreEntry * e)
{
    if (e->store_status != STORE_PENDING)
	return 0;
    return 1;
}

static void
statPendingObj(StoreEntry * sentry)
{
    statObjectsStart(sentry, statObjectsPendingFilter);
}

static int
statObjectsClientsFilter(const StoreEntry * e)
{
    if (e->mem_obj == NULL)
	return 0;
    if (e->mem_obj->clients.head == NULL)
	return 0;
    return 1;
}

static void
statClientsObj(StoreEntry * sentry)
{
    statObjectsStart(sentry, statObjectsClientsFilter);
}

#ifdef XMALLOC_STATISTICS
static void
info_get_mallstat(int size, int number, int oldnum, void *data)
{
    StoreEntry *sentry = data;
    if (number > 0)
	storeAppendPrintf(sentry, "%d\t %d\t %d\t %.1f\n", size, number, number - oldnum, xdiv((number - oldnum), xm_deltat));
}
#endif

static const char *
fdRemoteAddr(const fde * f)
{
    LOCAL_ARRAY(char, buf, 32);
    if (f->type != FD_SOCKET)
	return null_string;
    snprintf(buf, 32, "%s.%d", f->ipaddr, (int) f->remote_port);
    return buf;
}

static void
statFiledescriptors(StoreEntry * sentry)
{
    int i;
    fde *f;
    storeAppendPrintf(sentry, "Active file descriptors:\n");
    storeAppendPrintf(sentry, "%-4s %-6s %-4s %-7s* %-7s* %-21s %s\n",
	"File",
	"Type",
	"Tout",
	"Nread",
	"Nwrite",
	"Remote Address",
	"Description");
    storeAppendPrintf(sentry, "---- ------ ---- -------- -------- --------------------- ------------------------------\n");
    for (i = 0; i < Squid_MaxFD; i++) {
	f = &fd_table[i];
	if (!f->flags.open)
	    continue;
	storeAppendPrintf(sentry, "%4d %-6.6s %4d %7" PRINTF_OFF_T "%c %7" PRINTF_OFF_T "%c %-21s %s\n",
	    i,
	    fdTypeStr[f->type],
	    f->timeout_handler ? (int) (f->timeout - squid_curtime) / 60 : 0,
	    f->bytes_read,
	    f->read_handler ? '*' : ' ',
	    f->bytes_written,
	    f->write_handler ? '*' : ' ',
	    fdRemoteAddr(f),
	    f->desc);
    }
}

static void
info_get(StoreEntry * sentry)
{
    struct rusage rusage;
    double cputime;
    double runtime;
#if HAVE_MSTATS && HAVE_GNUMALLOC_H
    struct mstats ms;
#elif HAVE_MALLINFO && HAVE_STRUCT_MALLINFO
    struct mallinfo mp;
    int t;
#endif

    runtime = tvSubDsec(squid_start, current_time);
    if (runtime == 0.0)
	runtime = 1.0;
    storeAppendPrintf(sentry, "Squid Object Cache: Version %s\n",
	version_string);
    storeAppendPrintf(sentry, "Start Time:\t%s\n",
	mkrfc1123(squid_start.tv_sec));
    storeAppendPrintf(sentry, "Current Time:\t%s\n",
	mkrfc1123(current_time.tv_sec));
    storeAppendPrintf(sentry, "Connection information for %s:\n",
	appname);
    storeAppendPrintf(sentry, "\tNumber of clients accessing cache:\t%u\n",
	statCounter.client_http.clients);
    storeAppendPrintf(sentry, "\tNumber of HTTP requests received:\t%u\n",
	statCounter.client_http.requests);
    storeAppendPrintf(sentry, "\tNumber of ICP messages received:\t%u\n",
	statCounter.icp.pkts_recv);
    storeAppendPrintf(sentry, "\tNumber of ICP messages sent:\t%u\n",
	statCounter.icp.pkts_sent);
    storeAppendPrintf(sentry, "\tNumber of queued ICP replies:\t%u\n",
	statCounter.icp.replies_queued);
    storeAppendPrintf(sentry, "\tRequest failure ratio:\t%5.2f\n",
	request_failure_ratio);

    storeAppendPrintf(sentry, "\tAverage HTTP requests per minute since start:\t%.1f\n",
	statCounter.client_http.requests / (runtime / 60.0));
    storeAppendPrintf(sentry, "\tAverage ICP messages per minute since start:\t%.1f\n",
	(statCounter.icp.pkts_sent + statCounter.icp.pkts_recv) / (runtime / 60.0));
    storeAppendPrintf(sentry, "\tSelect loop called: %d times, %0.3f ms avg\n",
	statCounter.select_loops, 1000.0 * runtime / statCounter.select_loops);

    storeAppendPrintf(sentry, "Cache information for %s:\n",
	appname);
    storeAppendPrintf(sentry, "\tRequest Hit Ratios:\t5min: %3.1f%%, 60min: %3.1f%%\n",
	statRequestHitRatio(5),
	statRequestHitRatio(60));
    storeAppendPrintf(sentry, "\tByte Hit Ratios:\t5min: %3.1f%%, 60min: %3.1f%%\n",
	statByteHitRatio(5),
	statByteHitRatio(60));
    storeAppendPrintf(sentry, "\tRequest Memory Hit Ratios:\t5min: %3.1f%%, 60min: %3.1f%%\n",
	statRequestHitMemoryRatio(5),
	statRequestHitMemoryRatio(60));
    storeAppendPrintf(sentry, "\tRequest Disk Hit Ratios:\t5min: %3.1f%%, 60min: %3.1f%%\n",
	statRequestHitDiskRatio(5),
	statRequestHitDiskRatio(60));
    storeAppendPrintf(sentry, "\tStorage Swap size:\t%d KB\n",
	store_swap_size);
    storeAppendPrintf(sentry, "\tStorage Mem size:\t%d KB\n",
	(int) (store_mem_size >> 10));
    storeAppendPrintf(sentry, "\tMean Object Size:\t%0.2f KB\n",
	n_disk_objects ? (double) store_swap_size / n_disk_objects : 0.0);
    storeAppendPrintf(sentry, "\tRequests given to unlinkd:\t%d\n",
	statCounter.unlink.requests);

    storeAppendPrintf(sentry, "Median Service Times (seconds)  5 min    60 min:\n");
    storeAppendPrintf(sentry, "\tHTTP Requests (All):  %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_HTTP) / 1000.0,
	statMedianSvc(60, MEDIAN_HTTP) / 1000.0);
    storeAppendPrintf(sentry, "\tCache Misses:         %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_MISS) / 1000.0,
	statMedianSvc(60, MEDIAN_MISS) / 1000.0);
    storeAppendPrintf(sentry, "\tCache Hits:           %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_HIT) / 1000.0,
	statMedianSvc(60, MEDIAN_HIT) / 1000.0);
    storeAppendPrintf(sentry, "\tNear Hits:            %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_NH) / 1000.0,
	statMedianSvc(60, MEDIAN_NH) / 1000.0);
    storeAppendPrintf(sentry, "\tNot-Modified Replies: %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_NM) / 1000.0,
	statMedianSvc(60, MEDIAN_NM) / 1000.0);
    storeAppendPrintf(sentry, "\tDNS Lookups:          %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_DNS) / 1000.0,
	statMedianSvc(60, MEDIAN_DNS) / 1000.0);
    storeAppendPrintf(sentry, "\tICP Queries:          %8.5f %8.5f\n",
	statMedianSvc(5, MEDIAN_ICP_QUERY) / 1000000.0,
	statMedianSvc(60, MEDIAN_ICP_QUERY) / 1000000.0);

    squid_getrusage(&rusage);
    cputime = rusage_cputime(&rusage);
    storeAppendPrintf(sentry, "Resource usage for %s:\n", appname);
    storeAppendPrintf(sentry, "\tUP Time:\t%.3f seconds\n", runtime);
    storeAppendPrintf(sentry, "\tCPU Time:\t%.3f seconds\n", cputime);
    storeAppendPrintf(sentry, "\tCPU Usage:\t%.2f%%\n",
	dpercent(cputime, runtime));
    storeAppendPrintf(sentry, "\tCPU Usage, 5 minute avg:\t%.2f%%\n",
	statCPUUsage(5));
    storeAppendPrintf(sentry, "\tCPU Usage, 60 minute avg:\t%.2f%%\n",
	statCPUUsage(60));
#if HAVE_SBRK
    storeAppendPrintf(sentry, "\tProcess Data Segment Size via sbrk(): %d KB\n",
	(int) (((char *) sbrk(0) - (char *) sbrk_start) >> 10));
#endif
    storeAppendPrintf(sentry, "\tMaximum Resident Size: %d KB\n",
	rusage_maxrss(&rusage));
    storeAppendPrintf(sentry, "\tPage faults with physical i/o: %d\n",
	rusage_pagefaults(&rusage));

#if HAVE_MSTATS && HAVE_GNUMALLOC_H
    ms = mstats();
    storeAppendPrintf(sentry, "Memory usage for %s via mstats():\n",
	appname);
    storeAppendPrintf(sentry, "\tTotal space in arena:  %6d KB\n",
	ms.bytes_total >> 10);
    storeAppendPrintf(sentry, "\tTotal free:            %6d KB %d%%\n",
	ms.bytes_free >> 10, percent(ms.bytes_free, ms.bytes_total));
#elif HAVE_MALLINFO && HAVE_STRUCT_MALLINFO
    mp = mallinfo();
    storeAppendPrintf(sentry, "Memory usage for %s via mallinfo():\n",
	appname);
    storeAppendPrintf(sentry, "\tTotal space in arena:  %6ld KB\n",
	(long int) mp.arena >> 10);
    storeAppendPrintf(sentry, "\tOrdinary blocks:       %6ld KB %6ld blks\n",
	(long int) mp.uordblks >> 10, (long int) mp.ordblks);
    storeAppendPrintf(sentry, "\tSmall blocks:          %6ld KB %6ld blks\n",
	(long int) mp.usmblks >> 10, (long int) mp.smblks);
    storeAppendPrintf(sentry, "\tHolding blocks:        %6ld KB %6ld blks\n",
	(long int) mp.hblkhd >> 10, (long int) mp.hblks);
    storeAppendPrintf(sentry, "\tFree Small blocks:     %6ld KB\n",
	(long int) mp.fsmblks >> 10);
    storeAppendPrintf(sentry, "\tFree Ordinary blocks:  %6ld KB\n",
	(long int) mp.fordblks >> 10);
    t = mp.uordblks + mp.usmblks + mp.hblkhd;
    storeAppendPrintf(sentry, "\tTotal in use:          %6d KB %d%%\n",
	t >> 10, percent(t, mp.arena + mp.hblkhd));
    t = mp.fsmblks + mp.fordblks;
    storeAppendPrintf(sentry, "\tTotal free:            %6d KB %d%%\n",
	t >> 10, percent(t, mp.arena + mp.hblkhd));
    t = mp.arena + mp.hblkhd;
    storeAppendPrintf(sentry, "\tTotal size:            %6d KB\n",
	t >> 10);
#if HAVE_EXT_MALLINFO
    storeAppendPrintf(sentry, "\tmax size of small blocks:\t%d\n", mp.mxfast);
    storeAppendPrintf(sentry, "\tnumber of small blocks in a holding block:\t%d\n",
	mp.nlblks);
    storeAppendPrintf(sentry, "\tsmall block rounding factor:\t%d\n", mp.grain);
    storeAppendPrintf(sentry, "\tspace (including overhead) allocated in ord. blks:\t%d\n"
	,mp.uordbytes);
    storeAppendPrintf(sentry, "\tnumber of ordinary blocks allocated:\t%d\n",
	mp.allocated);
    storeAppendPrintf(sentry, "\tbytes used in maintaining the free tree:\t%d\n",
	mp.treeoverhead);
#endif /* HAVE_EXT_MALLINFO */
#endif /* HAVE_MALLINFO */
    storeAppendPrintf(sentry, "Memory accounted for:\n");
    storeAppendPrintf(sentry, "\tTotal accounted:       %6d KB\n",
	statMemoryAccounted() >> 10);
    storeAppendPrintf(sentry, "\tmemPoolAlloc calls: %u\n",
	mem_pool_alloc_calls);
    storeAppendPrintf(sentry, "\tmemPoolFree calls: %u\n",
	mem_pool_free_calls);

    storeAppendPrintf(sentry, "File descriptor usage for %s:\n", appname);
    storeAppendPrintf(sentry, "\tMaximum number of file descriptors:   %4d\n",
	Squid_MaxFD);
    storeAppendPrintf(sentry, "\tLargest file desc currently in use:   %4d\n",
	Biggest_FD);
    storeAppendPrintf(sentry, "\tNumber of file desc currently in use: %4d\n",
	Number_FD);
    storeAppendPrintf(sentry, "\tFiles queued for open:                %4d\n",
	Opening_FD);
    storeAppendPrintf(sentry, "\tAvailable number of file descriptors: %4d\n",
	fdNFree());
    storeAppendPrintf(sentry, "\tReserved number of file descriptors:  %4d\n",
	RESERVED_FD);
    storeAppendPrintf(sentry, "\tStore Disk files open:                %4d\n",
	store_open_disk_fd);

    storeAppendPrintf(sentry, "Internal Data Structures:\n");
    storeAppendPrintf(sentry, "\t%6d StoreEntries\n",
	memInUse(MEM_STOREENTRY));
    storeAppendPrintf(sentry, "\t%6d StoreEntries with MemObjects\n",
	memInUse(MEM_MEMOBJECT));
    storeAppendPrintf(sentry, "\t%6d Hot Object Cache Items\n",
	hot_obj_count);
    storeAppendPrintf(sentry, "\t%6d on-disk objects\n",
	n_disk_objects);

#if XMALLOC_STATISTICS
    xm_deltat = current_dtime - xm_time;
    xm_time = current_dtime;
    storeAppendPrintf(sentry, "\nMemory allocation statistics\n");
    storeAppendPrintf(sentry, "Allocation Size\t Alloc Count\t Alloc Delta\t Allocs/sec \n");
    malloc_statistics(info_get_mallstat, sentry);
#endif
}

#define XAVG(X) (dt ? (double) (f->X - l->X) / dt : 0.0)
static void
statAvgDump(StoreEntry * sentry, int minutes, int hours)
{
    StatCounters *f;
    StatCounters *l;
    double dt;
    double ct;
    double x;
    assert(N_COUNT_HIST > 1);
    assert(minutes > 0 || hours > 0);
    f = &CountHist[0];
    l = f;
    if (minutes > 0 && hours == 0) {
	/* checking minute readings ... */
	if (minutes > N_COUNT_HIST - 1)
	    minutes = N_COUNT_HIST - 1;
	l = &CountHist[minutes];
    } else if (minutes == 0 && hours > 0) {
	/* checking hour readings ... */
	if (hours > N_COUNT_HOUR_HIST - 1)
	    hours = N_COUNT_HOUR_HIST - 1;
	l = &CountHourHist[hours];
    } else {
	debug(18, 1) ("statAvgDump: Invalid args, minutes=%d, hours=%d\n",
	    minutes, hours);
	return;
    }
    dt = tvSubDsec(l->timestamp, f->timestamp);
    ct = f->cputime - l->cputime;

    storeAppendPrintf(sentry, "sample_start_time = %d.%d (%s)\n",
	(int) l->timestamp.tv_sec,
	(int) l->timestamp.tv_usec,
	mkrfc1123(l->timestamp.tv_sec));
    storeAppendPrintf(sentry, "sample_end_time = %d.%d (%s)\n",
	(int) f->timestamp.tv_sec,
	(int) f->timestamp.tv_usec,
	mkrfc1123(f->timestamp.tv_sec));

    storeAppendPrintf(sentry, "client_http.requests = %f/sec\n",
	XAVG(client_http.requests));
    storeAppendPrintf(sentry, "client_http.hits = %f/sec\n",
	XAVG(client_http.hits));
    storeAppendPrintf(sentry, "client_http.errors = %f/sec\n",
	XAVG(client_http.errors));
    storeAppendPrintf(sentry, "client_http.kbytes_in = %f/sec\n",
	XAVG(client_http.kbytes_in.kb));
    storeAppendPrintf(sentry, "client_http.kbytes_out = %f/sec\n",
	XAVG(client_http.kbytes_out.kb));

    x = statHistDeltaMedian(&l->client_http.all_svc_time,
	&f->client_http.all_svc_time);
    storeAppendPrintf(sentry, "client_http.all_median_svc_time = %f seconds\n",
	x / 1000.0);
    x = statHistDeltaMedian(&l->client_http.miss_svc_time,
	&f->client_http.miss_svc_time);
    storeAppendPrintf(sentry, "client_http.miss_median_svc_time = %f seconds\n",
	x / 1000.0);
    x = statHistDeltaMedian(&l->client_http.nm_svc_time,
	&f->client_http.nm_svc_time);
    storeAppendPrintf(sentry, "client_http.nm_median_svc_time = %f seconds\n",
	x / 1000.0);
    x = statHistDeltaMedian(&l->client_http.nh_svc_time,
	&f->client_http.nh_svc_time);
    storeAppendPrintf(sentry, "client_http.nh_median_svc_time = %f seconds\n",
	x / 1000.0);
    x = statHistDeltaMedian(&l->client_http.hit_svc_time,
	&f->client_http.hit_svc_time);
    storeAppendPrintf(sentry, "client_http.hit_median_svc_time = %f seconds\n",
	x / 1000.0);

    storeAppendPrintf(sentry, "server.all.requests = %f/sec\n",
	XAVG(server.all.requests));
    storeAppendPrintf(sentry, "server.all.errors = %f/sec\n",
	XAVG(server.all.errors));
    storeAppendPrintf(sentry, "server.all.kbytes_in = %f/sec\n",
	XAVG(server.all.kbytes_in.kb));
    storeAppendPrintf(sentry, "server.all.kbytes_out = %f/sec\n",
	XAVG(server.all.kbytes_out.kb));

    storeAppendPrintf(sentry, "server.http.requests = %f/sec\n",
	XAVG(server.http.requests));
    storeAppendPrintf(sentry, "server.http.errors = %f/sec\n",
	XAVG(server.http.errors));
    storeAppendPrintf(sentry, "server.http.kbytes_in = %f/sec\n",
	XAVG(server.http.kbytes_in.kb));
    storeAppendPrintf(sentry, "server.http.kbytes_out = %f/sec\n",
	XAVG(server.http.kbytes_out.kb));

    storeAppendPrintf(sentry, "server.ftp.requests = %f/sec\n",
	XAVG(server.ftp.requests));
    storeAppendPrintf(sentry, "server.ftp.errors = %f/sec\n",
	XAVG(server.ftp.errors));
    storeAppendPrintf(sentry, "server.ftp.kbytes_in = %f/sec\n",
	XAVG(server.ftp.kbytes_in.kb));
    storeAppendPrintf(sentry, "server.ftp.kbytes_out = %f/sec\n",
	XAVG(server.ftp.kbytes_out.kb));

    storeAppendPrintf(sentry, "server.other.requests = %f/sec\n",
	XAVG(server.other.requests));
    storeAppendPrintf(sentry, "server.other.errors = %f/sec\n",
	XAVG(server.other.errors));
    storeAppendPrintf(sentry, "server.other.kbytes_in = %f/sec\n",
	XAVG(server.other.kbytes_in.kb));
    storeAppendPrintf(sentry, "server.other.kbytes_out = %f/sec\n",
	XAVG(server.other.kbytes_out.kb));

#ifdef HS_FEAT_ICAP
    storeAppendPrintf(sentry, "icap.all.requests = %f/sec\n",
	XAVG(icap.all.requests));
    storeAppendPrintf(sentry, "icap.all.errors = %f/sec\n",
	XAVG(icap.all.errors));
    storeAppendPrintf(sentry, "icap.all.kbytes_in = %f/sec\n",
	XAVG(icap.all.kbytes_in.kb));
    storeAppendPrintf(sentry, "icap.all.kbytes_out = %f/sec\n",
	XAVG(icap.all.kbytes_out.kb));
#endif

    storeAppendPrintf(sentry, "icp.pkts_sent = %f/sec\n",
	XAVG(icp.pkts_sent));
    storeAppendPrintf(sentry, "icp.pkts_recv = %f/sec\n",
	XAVG(icp.pkts_recv));
    storeAppendPrintf(sentry, "icp.queries_sent = %f/sec\n",
	XAVG(icp.queries_sent));
    storeAppendPrintf(sentry, "icp.replies_sent = %f/sec\n",
	XAVG(icp.replies_sent));
    storeAppendPrintf(sentry, "icp.queries_recv = %f/sec\n",
	XAVG(icp.queries_recv));
    storeAppendPrintf(sentry, "icp.replies_recv = %f/sec\n",
	XAVG(icp.replies_recv));
    storeAppendPrintf(sentry, "icp.replies_queued = %f/sec\n",
	XAVG(icp.replies_queued));
    storeAppendPrintf(sentry, "icp.query_timeouts = %f/sec\n",
	XAVG(icp.query_timeouts));
    storeAppendPrintf(sentry, "icp.kbytes_sent = %f/sec\n",
	XAVG(icp.kbytes_sent.kb));
    storeAppendPrintf(sentry, "icp.kbytes_recv = %f/sec\n",
	XAVG(icp.kbytes_recv.kb));
    storeAppendPrintf(sentry, "icp.q_kbytes_sent = %f/sec\n",
	XAVG(icp.q_kbytes_sent.kb));
    storeAppendPrintf(sentry, "icp.r_kbytes_sent = %f/sec\n",
	XAVG(icp.r_kbytes_sent.kb));
    storeAppendPrintf(sentry, "icp.q_kbytes_recv = %f/sec\n",
	XAVG(icp.q_kbytes_recv.kb));
    storeAppendPrintf(sentry, "icp.r_kbytes_recv = %f/sec\n",
	XAVG(icp.r_kbytes_recv.kb));
    x = statHistDeltaMedian(&l->icp.query_svc_time, &f->icp.query_svc_time);
    storeAppendPrintf(sentry, "icp.query_median_svc_time = %f seconds\n",
	x / 1000000.0);
    x = statHistDeltaMedian(&l->icp.reply_svc_time, &f->icp.reply_svc_time);
    storeAppendPrintf(sentry, "icp.reply_median_svc_time = %f seconds\n",
	x / 1000000.0);
    x = statHistDeltaMedian(&l->dns.svc_time, &f->dns.svc_time);
    storeAppendPrintf(sentry, "dns.median_svc_time = %f seconds\n",
	x / 1000.0);
    storeAppendPrintf(sentry, "unlink.requests = %f/sec\n",
	XAVG(unlink.requests));
    storeAppendPrintf(sentry, "page_faults = %f/sec\n",
	XAVG(page_faults));
    storeAppendPrintf(sentry, "select_loops = %f/sec\n",
	XAVG(select_loops));
    storeAppendPrintf(sentry, "select_fds = %f/sec\n",
	XAVG(select_fds));
    storeAppendPrintf(sentry, "average_select_fd_period = %f/fd\n",
	f->select_fds > l->select_fds ?
	(f->select_time - l->select_time) / (f->select_fds - l->select_fds)
	: 0.0);
    x = statHistDeltaMedian(&l->select_fds_hist, &f->select_fds_hist);
    storeAppendPrintf(sentry, "median_select_fds = %f\n", x);
    storeAppendPrintf(sentry, "swap.outs = %f/sec\n",
	XAVG(swap.outs));
    storeAppendPrintf(sentry, "swap.ins = %f/sec\n",
	XAVG(swap.ins));
    storeAppendPrintf(sentry, "swap.files_cleaned = %f/sec\n",
	XAVG(swap.files_cleaned));
    storeAppendPrintf(sentry, "aborted_requests = %f/sec\n",
	XAVG(aborted_requests));

#if HAVE_POLL
    storeAppendPrintf(sentry, "syscalls.polls = %f/sec\n", XAVG(syscalls.polls));
#else
    storeAppendPrintf(sentry, "syscalls.selects = %f/sec\n", XAVG(syscalls.selects));
#endif
    storeAppendPrintf(sentry, "syscalls.disk.opens = %f/sec\n", XAVG(syscalls.disk.opens));
    storeAppendPrintf(sentry, "syscalls.disk.closes = %f/sec\n", XAVG(syscalls.disk.closes));
    storeAppendPrintf(sentry, "syscalls.disk.reads = %f/sec\n", XAVG(syscalls.disk.reads));
    storeAppendPrintf(sentry, "syscalls.disk.writes = %f/sec\n", XAVG(syscalls.disk.writes));
    storeAppendPrintf(sentry, "syscalls.disk.seeks = %f/sec\n", XAVG(syscalls.disk.seeks));
    storeAppendPrintf(sentry, "syscalls.disk.unlinks = %f/sec\n", XAVG(syscalls.disk.unlinks));
    storeAppendPrintf(sentry, "syscalls.sock.accepts = %f/sec\n", XAVG(syscalls.sock.accepts));
    storeAppendPrintf(sentry, "syscalls.sock.sockets = %f/sec\n", XAVG(syscalls.sock.sockets));
    storeAppendPrintf(sentry, "syscalls.sock.connects = %f/sec\n", XAVG(syscalls.sock.connects));
    storeAppendPrintf(sentry, "syscalls.sock.binds = %f/sec\n", XAVG(syscalls.sock.binds));
    storeAppendPrintf(sentry, "syscalls.sock.closes = %f/sec\n", XAVG(syscalls.sock.closes));
    storeAppendPrintf(sentry, "syscalls.sock.reads = %f/sec\n", XAVG(syscalls.sock.reads));
    storeAppendPrintf(sentry, "syscalls.sock.writes = %f/sec\n", XAVG(syscalls.sock.writes));
    storeAppendPrintf(sentry, "syscalls.sock.recvfroms = %f/sec\n", XAVG(syscalls.sock.recvfroms));
    storeAppendPrintf(sentry, "syscalls.sock.sendtos = %f/sec\n", XAVG(syscalls.sock.sendtos));

    storeAppendPrintf(sentry, "cpu_time = %f seconds\n", ct);
    storeAppendPrintf(sentry, "wall_time = %f seconds\n", dt);
    storeAppendPrintf(sentry, "cpu_usage = %f%%\n", dpercent(ct, dt));
}


void
statInit(void)
{
    int i;
    debug(18, 5) ("statInit: Initializing...\n");
    CBDATA_INIT_TYPE(StatObjectsState);
    for (i = 0; i < N_COUNT_HIST; i++)
	statCountersInit(&CountHist[i]);
    for (i = 0; i < N_COUNT_HOUR_HIST; i++)
	statCountersInit(&CountHourHist[i]);
    statCountersInit(&statCounter);
    eventAdd("statAvgTick", statAvgTick, NULL, (double) COUNT_INTERVAL, 1);
    cachemgrRegister("info",
	"General Runtime Information",
	info_get, 0, 1);
    cachemgrRegister("filedescriptors",
	"Process Filedescriptor Allocation",
	statFiledescriptors, 0, 1);
    cachemgrRegister("objects",
	"All Cache Objects",
	stat_objects_get, 0, 0);
    cachemgrRegister("vm_objects",
	"In-Memory and In-Transit Objects",
	stat_vmobjects_get, 0, 0);
    cachemgrRegister("openfd_objects",
	"Objects with Swapout files open",
	statOpenfdObj, 0, 0);
    cachemgrRegister("pending_objects",
	"Objects being retreived from the network",
	statPendingObj, 0, 0);
    cachemgrRegister("client_objects",
	"Objects being sent to clients",
	statClientsObj, 0, 0);
    cachemgrRegister("io",
	"Server-side network read() size histograms",
	stat_io_get, 0, 1);
    cachemgrRegister("counters",
	"Traffic and Resource Counters",
	statCountersDump, 0, 1);
    cachemgrRegister("peer_select",
	"Peer Selection Algorithms",
	statPeerSelect, 0, 1);
    cachemgrRegister("digest_stats",
	"Cache Digest and ICP blob",
	statDigestBlob, 0, 1);
    cachemgrRegister("5min",
	"5 Minute Average of Counters",
	statAvg5min, 0, 1);
    cachemgrRegister("60min",
	"60 Minute Average of Counters",
	statAvg60min, 0, 1);
    cachemgrRegister("utilization",
	"Cache Utilization",
	statUtilization, 0, 1);
#if STAT_GRAPHS
    cachemgrRegister("graph_variables",
	"Display cache metrics graphically",
	statGraphDump, 0, 1);
#endif
    cachemgrRegister("histograms",
	"Full Histogram Counts",
	statCountersHistograms, 0, 1);
    ClientActiveRequests.head = NULL;
    ClientActiveRequests.tail = NULL;
    cachemgrRegister("active_requests",
	"Client-side Active Requests",
	statClientRequests, 0, 1);
}

static void
statAvgTick(void *notused)
{
    StatCounters *t = &CountHist[0];
    StatCounters *p = &CountHist[1];
    StatCounters *c = &statCounter;
    struct rusage rusage;
    eventAdd("statAvgTick", statAvgTick, NULL, (double) COUNT_INTERVAL, 1);
    squid_getrusage(&rusage);
    c->page_faults = rusage_pagefaults(&rusage);
    c->cputime = rusage_cputime(&rusage);
    c->timestamp = current_time;
    /* even if NCountHist is small, we already Init()ed the tail */
    statCountersClean(CountHist + N_COUNT_HIST - 1);
    xmemmove(p, t, (N_COUNT_HIST - 1) * sizeof(StatCounters));
    statCountersCopy(t, c);
    NCountHist++;

    if ((NCountHist % COUNT_INTERVAL) == 0) {
	/* we have an hours worth of readings.  store previous hour */
	StatCounters *t = &CountHourHist[0];
	StatCounters *p = &CountHourHist[1];
	StatCounters *c = &CountHist[N_COUNT_HIST - 1];
	statCountersClean(CountHourHist + N_COUNT_HOUR_HIST - 1);
	xmemmove(p, t, (N_COUNT_HOUR_HIST - 1) * sizeof(StatCounters));
	statCountersCopy(t, c);
	NCountHourHist++;
    }
    if (Config.warnings.high_rptm > 0) {
	int i = (int) statMedianSvc(20, MEDIAN_HTTP);
	if (Config.warnings.high_rptm < i)
	    debug(18, 0) ("WARNING: Median response time is %d milliseconds\n", i);
    }
    if (Config.warnings.high_pf) {
	int i = (CountHist[0].page_faults - CountHist[1].page_faults);
	double dt = tvSubDsec(CountHist[0].timestamp, CountHist[1].timestamp);
	if (i > 0 && dt > 0.0) {
	    i /= (int) dt;
	    if (Config.warnings.high_pf < i)
		debug(18, 0) ("WARNING: Page faults occuring at %d/sec\n", i);
	}
    }
    if (Config.warnings.high_memory) {
	int i = 0;
#if HAVE_MSTATS && HAVE_GNUMALLOC_H
	struct mstats ms = mstats();
	i = ms.bytes_total;
#elif HAVE_MALLINFO && HAVE_STRUCT_MALLINFO
	struct mallinfo mp = mallinfo();
	i = mp.arena;
#elif HAVE_SBRK
	i = (size_t) ((char *) sbrk(0) - (char *) sbrk_start);
#endif
	if (Config.warnings.high_memory < i)
	    debug(18, 0) ("WARNING: Memory usage at %d MB\n", i >> 20);
    }
}

static void
statCountersInit(StatCounters * C)
{
    assert(C);
    memset(C, 0, sizeof(*C));
    C->timestamp = current_time;
    statCountersInitSpecial(C);
}

/* add special cases here as they arrive */
static void
statCountersInitSpecial(StatCounters * C)
{
    /*
     * HTTP svc_time hist is kept in milli-seconds; max of 3 hours.
     */
    statHistLogInit(&C->client_http.all_svc_time, 300, 0.0, 3600000.0 * 3.0);
    statHistLogInit(&C->client_http.miss_svc_time, 300, 0.0, 3600000.0 * 3.0);
    statHistLogInit(&C->client_http.nm_svc_time, 300, 0.0, 3600000.0 * 3.0);
    statHistLogInit(&C->client_http.nh_svc_time, 300, 0.0, 3600000.0 * 3.0);
    statHistLogInit(&C->client_http.hit_svc_time, 300, 0.0, 3600000.0 * 3.0);
    /*
     * ICP svc_time hist is kept in micro-seconds; max of 1 minute.
     */
    statHistLogInit(&C->icp.query_svc_time, 300, 0.0, 1000000.0 * 60.0);
    statHistLogInit(&C->icp.reply_svc_time, 300, 0.0, 1000000.0 * 60.0);
    /*
     * DNS svc_time hist is kept in milli-seconds; max of 10 minutes.
     */
    statHistLogInit(&C->dns.svc_time, 300, 0.0, 60000.0 * 10.0);
    /*
     * Cache Digest Stuff
     */
    statHistEnumInit(&C->cd.on_xition_count, CacheDigestHashFuncCount);
    statHistEnumInit(&C->comm_icp_incoming, INCOMING_ICP_MAX);
    statHistEnumInit(&C->comm_dns_incoming, INCOMING_DNS_MAX);
    statHistEnumInit(&C->comm_http_incoming, INCOMING_HTTP_MAX);
    statHistIntInit(&C->select_fds_hist, 256);	/* was SQUID_MAXFD, but it is way too much. It is OK to crop this statistics */
}

/* add special cases here as they arrive */
static void
statCountersClean(StatCounters * C)
{
    assert(C);
    statHistClean(&C->client_http.all_svc_time);
    statHistClean(&C->client_http.miss_svc_time);
    statHistClean(&C->client_http.nm_svc_time);
    statHistClean(&C->client_http.nh_svc_time);
    statHistClean(&C->client_http.hit_svc_time);
    statHistClean(&C->icp.query_svc_time);
    statHistClean(&C->icp.reply_svc_time);
    statHistClean(&C->dns.svc_time);
    statHistClean(&C->cd.on_xition_count);
    statHistClean(&C->comm_icp_incoming);
    statHistClean(&C->comm_dns_incoming);
    statHistClean(&C->comm_http_incoming);
    statHistClean(&C->select_fds_hist);
}

/* add special cases here as they arrive */
static void
statCountersCopy(StatCounters * dest, const StatCounters * orig)
{
    assert(dest && orig);
    /* this should take care of all the fields, but "special" ones */
    xmemcpy(dest, orig, sizeof(*dest));
    /* prepare space where to copy special entries */
    statCountersInitSpecial(dest);
    /* now handle special cases */
    /* note: we assert that histogram capacities do not change */
    statHistCopy(&dest->client_http.all_svc_time, &orig->client_http.all_svc_time);
    statHistCopy(&dest->client_http.miss_svc_time, &orig->client_http.miss_svc_time);
    statHistCopy(&dest->client_http.nm_svc_time, &orig->client_http.nm_svc_time);
    statHistCopy(&dest->client_http.nh_svc_time, &orig->client_http.nh_svc_time);
    statHistCopy(&dest->client_http.hit_svc_time, &orig->client_http.hit_svc_time);
    statHistCopy(&dest->icp.query_svc_time, &orig->icp.query_svc_time);
    statHistCopy(&dest->icp.reply_svc_time, &orig->icp.reply_svc_time);
    statHistCopy(&dest->dns.svc_time, &orig->dns.svc_time);
    statHistCopy(&dest->cd.on_xition_count, &orig->cd.on_xition_count);
    statHistCopy(&dest->comm_icp_incoming, &orig->comm_icp_incoming);
    statHistCopy(&dest->comm_http_incoming, &orig->comm_http_incoming);
    statHistCopy(&dest->select_fds_hist, &orig->select_fds_hist);
}

static void
statCountersHistograms(StoreEntry * sentry)
{
    StatCounters *f = &statCounter;
    storeAppendPrintf(sentry, "client_http.all_svc_time histogram:\n");
    statHistDump(&f->client_http.all_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "client_http.miss_svc_time histogram:\n");
    statHistDump(&f->client_http.miss_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "client_http.nm_svc_time histogram:\n");
    statHistDump(&f->client_http.nm_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "client_http.nh_svc_time histogram:\n");
    statHistDump(&f->client_http.nh_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "client_http.hit_svc_time histogram:\n");
    statHistDump(&f->client_http.hit_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "icp.query_svc_time histogram:\n");
    statHistDump(&f->icp.query_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "icp.reply_svc_time histogram:\n");
    statHistDump(&f->icp.reply_svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "dns.svc_time histogram:\n");
    statHistDump(&f->dns.svc_time, sentry, NULL);
    storeAppendPrintf(sentry, "select_fds_hist histogram:\n");
    statHistDump(&f->select_fds_hist, sentry, NULL);
}

static void
statCountersDump(StoreEntry * sentry)
{
    StatCounters *f = &statCounter;
    struct rusage rusage;
    squid_getrusage(&rusage);
    f->page_faults = rusage_pagefaults(&rusage);
    f->cputime = rusage_cputime(&rusage);

    storeAppendPrintf(sentry, "sample_time = %d.%d (%s)\n",
	(int) f->timestamp.tv_sec,
	(int) f->timestamp.tv_usec,
	mkrfc1123(f->timestamp.tv_sec));
    storeAppendPrintf(sentry, "client_http.requests = %d\n",
	f->client_http.requests);
    storeAppendPrintf(sentry, "client_http.hits = %d\n",
	f->client_http.hits);
    storeAppendPrintf(sentry, "client_http.errors = %d\n",
	f->client_http.errors);
    storeAppendPrintf(sentry, "client_http.kbytes_in = %d\n",
	(int) f->client_http.kbytes_in.kb);
    storeAppendPrintf(sentry, "client_http.kbytes_out = %d\n",
	(int) f->client_http.kbytes_out.kb);
    storeAppendPrintf(sentry, "client_http.hit_kbytes_out = %d\n",
	(int) f->client_http.hit_kbytes_out.kb);

    storeAppendPrintf(sentry, "server.all.requests = %d\n",
	(int) f->server.all.requests);
    storeAppendPrintf(sentry, "server.all.errors = %d\n",
	(int) f->server.all.errors);
    storeAppendPrintf(sentry, "server.all.kbytes_in = %d\n",
	(int) f->server.all.kbytes_in.kb);
    storeAppendPrintf(sentry, "server.all.kbytes_out = %d\n",
	(int) f->server.all.kbytes_out.kb);

    storeAppendPrintf(sentry, "server.http.requests = %d\n",
	(int) f->server.http.requests);
    storeAppendPrintf(sentry, "server.http.errors = %d\n",
	(int) f->server.http.errors);
    storeAppendPrintf(sentry, "server.http.kbytes_in = %d\n",
	(int) f->server.http.kbytes_in.kb);
    storeAppendPrintf(sentry, "server.http.kbytes_out = %d\n",
	(int) f->server.http.kbytes_out.kb);

    storeAppendPrintf(sentry, "server.ftp.requests = %d\n",
	(int) f->server.ftp.requests);
    storeAppendPrintf(sentry, "server.ftp.errors = %d\n",
	(int) f->server.ftp.errors);
    storeAppendPrintf(sentry, "server.ftp.kbytes_in = %d\n",
	(int) f->server.ftp.kbytes_in.kb);
    storeAppendPrintf(sentry, "server.ftp.kbytes_out = %d\n",
	(int) f->server.ftp.kbytes_out.kb);

    storeAppendPrintf(sentry, "server.other.requests = %d\n",
	(int) f->server.other.requests);
    storeAppendPrintf(sentry, "server.other.errors = %d\n",
	(int) f->server.other.errors);
    storeAppendPrintf(sentry, "server.other.kbytes_in = %d\n",
	(int) f->server.other.kbytes_in.kb);
    storeAppendPrintf(sentry, "server.other.kbytes_out = %d\n",
	(int) f->server.other.kbytes_out.kb);

#if HS_FEAT_ICAP
    storeAppendPrintf(sentry, "icap.all.requests = %d\n",
	(int) f->icap.all.requests);
    storeAppendPrintf(sentry, "icap.all.errors = %d\n",
	(int) f->icap.all.errors);
    storeAppendPrintf(sentry, "icap.all.kbytes_in = %d\n",
	(int) f->icap.all.kbytes_in.kb);
    storeAppendPrintf(sentry, "icap.all.kbytes_out = %d\n",
	(int) f->icap.all.kbytes_out.kb);
#endif

    storeAppendPrintf(sentry, "icp.pkts_sent = %d\n",
	f->icp.pkts_sent);
    storeAppendPrintf(sentry, "icp.pkts_recv = %d\n",
	f->icp.pkts_recv);
    storeAppendPrintf(sentry, "icp.queries_sent = %d\n",
	f->icp.queries_sent);
    storeAppendPrintf(sentry, "icp.replies_sent = %d\n",
	f->icp.replies_sent);
    storeAppendPrintf(sentry, "icp.queries_recv = %d\n",
	f->icp.queries_recv);
    storeAppendPrintf(sentry, "icp.replies_recv = %d\n",
	f->icp.replies_recv);
    storeAppendPrintf(sentry, "icp.query_timeouts = %d\n",
	f->icp.query_timeouts);
    storeAppendPrintf(sentry, "icp.replies_queued = %d\n",
	f->icp.replies_queued);
    storeAppendPrintf(sentry, "icp.kbytes_sent = %d\n",
	(int) f->icp.kbytes_sent.kb);
    storeAppendPrintf(sentry, "icp.kbytes_recv = %d\n",
	(int) f->icp.kbytes_recv.kb);
    storeAppendPrintf(sentry, "icp.q_kbytes_sent = %d\n",
	(int) f->icp.q_kbytes_sent.kb);
    storeAppendPrintf(sentry, "icp.r_kbytes_sent = %d\n",
	(int) f->icp.r_kbytes_sent.kb);
    storeAppendPrintf(sentry, "icp.q_kbytes_recv = %d\n",
	(int) f->icp.q_kbytes_recv.kb);
    storeAppendPrintf(sentry, "icp.r_kbytes_recv = %d\n",
	(int) f->icp.r_kbytes_recv.kb);

#if USE_CACHE_DIGESTS
    storeAppendPrintf(sentry, "icp.times_used = %d\n",
	f->icp.times_used);
    storeAppendPrintf(sentry, "cd.times_used = %d\n",
	f->cd.times_used);
    storeAppendPrintf(sentry, "cd.msgs_sent = %d\n",
	f->cd.msgs_sent);
    storeAppendPrintf(sentry, "cd.msgs_recv = %d\n",
	f->cd.msgs_recv);
    storeAppendPrintf(sentry, "cd.memory = %d\n",
	(int) f->cd.memory.kb);
    storeAppendPrintf(sentry, "cd.local_memory = %d\n",
	(int) (store_digest ? store_digest->mask_size / 1024 : 0));
    storeAppendPrintf(sentry, "cd.kbytes_sent = %d\n",
	(int) f->cd.kbytes_sent.kb);
    storeAppendPrintf(sentry, "cd.kbytes_recv = %d\n",
	(int) f->cd.kbytes_recv.kb);
#endif

    storeAppendPrintf(sentry, "unlink.requests = %d\n",
	f->unlink.requests);
    storeAppendPrintf(sentry, "page_faults = %d\n",
	f->page_faults);
    storeAppendPrintf(sentry, "select_loops = %d\n",
	f->select_loops);
    storeAppendPrintf(sentry, "cpu_time = %f\n",
	f->cputime);
    storeAppendPrintf(sentry, "wall_time = %f\n",
	tvSubDsec(f->timestamp, current_time));
    storeAppendPrintf(sentry, "swap.outs = %d\n",
	f->swap.outs);
    storeAppendPrintf(sentry, "swap.ins = %d\n",
	f->swap.ins);
    storeAppendPrintf(sentry, "swap.files_cleaned = %d\n",
	f->swap.files_cleaned);
    storeAppendPrintf(sentry, "aborted_requests = %d\n",
	f->aborted_requests);
}

void
statFreeMemory(void)
{
    int i;
    for (i = 0; i < N_COUNT_HIST; i++)
	statCountersClean(&CountHist[i]);
    for (i = 0; i < N_COUNT_HOUR_HIST; i++)
	statCountersClean(&CountHourHist[i]);
}

static void
statPeerSelect(StoreEntry * sentry)
{
#if USE_CACHE_DIGESTS
    StatCounters *f = &statCounter;
    peer *peer;
    const int tot_used = f->cd.times_used + f->icp.times_used;

    /* totals */
    cacheDigestGuessStatsReport(&f->cd.guess, sentry, "all peers");
    /* per-peer */
    storeAppendPrintf(sentry, "\nPer-peer statistics:\n");
    for (peer = getFirstPeer(); peer; peer = getNextPeer(peer)) {
	if (peer->digest)
	    peerDigestStatsReport(peer->digest, sentry);
	else
	    storeAppendPrintf(sentry, "\nNo peer digest from %s\n", peer->host);
	storeAppendPrintf(sentry, "\n");
    }

    storeAppendPrintf(sentry, "\nAlgorithm usage:\n");
    storeAppendPrintf(sentry, "Cache Digest: %7d (%3d%%)\n",
	f->cd.times_used, xpercentInt(f->cd.times_used, tot_used));
    storeAppendPrintf(sentry, "Icp:          %7d (%3d%%)\n",
	f->icp.times_used, xpercentInt(f->icp.times_used, tot_used));
    storeAppendPrintf(sentry, "Total:        %7d (%3d%%)\n",
	tot_used, xpercentInt(tot_used, tot_used));
#else
    storeAppendPrintf(sentry, "peer digests are disabled; no stats is available.\n");
#endif
}

static void
statDigestBlob(StoreEntry * sentry)
{
    storeAppendPrintf(sentry, "\nCounters:\n");
    statCountersDump(sentry);
    storeAppendPrintf(sentry, "\n5 Min Averages:\n");
    statAvgDump(sentry, 5, 0);
    storeAppendPrintf(sentry, "\nHistograms:\n");
    statCountersHistograms(sentry);
    storeAppendPrintf(sentry, "\nPeer Digests:\n");
    statPeerSelect(sentry);
    storeAppendPrintf(sentry, "\nLocal Digest:\n");
    storeDigestReport(sentry);
}

static void
statAvg5min(StoreEntry * e)
{
    statAvgDump(e, 5, 0);
}

static void
statAvg60min(StoreEntry * e)
{
    statAvgDump(e, 60, 0);
}

static double
statMedianSvc(int interval, int which)
{
    StatCounters *f;
    StatCounters *l;
    double x;
    assert(interval > 0);
    if (interval > N_COUNT_HIST - 1)
	interval = N_COUNT_HIST - 1;
    f = &CountHist[0];
    l = &CountHist[interval];
    assert(f);
    assert(l);
    switch (which) {
    case MEDIAN_HTTP:
	x = statHistDeltaMedian(&l->client_http.all_svc_time, &f->client_http.all_svc_time);
	break;
    case MEDIAN_HIT:
	x = statHistDeltaMedian(&l->client_http.hit_svc_time, &f->client_http.hit_svc_time);
	break;
    case MEDIAN_MISS:
	x = statHistDeltaMedian(&l->client_http.miss_svc_time, &f->client_http.miss_svc_time);
	break;
    case MEDIAN_NM:
	x = statHistDeltaMedian(&l->client_http.nm_svc_time, &f->client_http.nm_svc_time);
	break;
    case MEDIAN_NH:
	x = statHistDeltaMedian(&l->client_http.nh_svc_time, &f->client_http.nh_svc_time);
	break;
    case MEDIAN_ICP_QUERY:
	x = statHistDeltaMedian(&l->icp.query_svc_time, &f->icp.query_svc_time);
	break;
    case MEDIAN_DNS:
	x = statHistDeltaMedian(&l->dns.svc_time, &f->dns.svc_time);
	break;
    default:
	debug(49, 5) ("get_median_val: unknown type.\n");
	x = 0;
    }
    return x;
}

/*
 * SNMP wants ints, ick
 */
#if UNUSED_CODE
int
get_median_svc(int interval, int which)
{
    return (int) statMedianSvc(interval, which);
}

#endif

StatCounters *
snmpStatGet(int minutes)
{
    return &CountHist[minutes];
}

int
stat5minClientRequests(void)
{
    assert(N_COUNT_HIST > 5);
    return statCounter.client_http.requests - CountHist[5].client_http.requests;
}

static double
statCPUUsage(int minutes)
{
    assert(minutes < N_COUNT_HIST);
    return dpercent(CountHist[0].cputime - CountHist[minutes].cputime,
	tvSubDsec(CountHist[minutes].timestamp, CountHist[0].timestamp));
}

extern double
statRequestHitRatio(int minutes)
{
    assert(minutes < N_COUNT_HIST);
    return dpercent(CountHist[0].client_http.hits -
	CountHist[minutes].client_http.hits,
	CountHist[0].client_http.requests -
	CountHist[minutes].client_http.requests);
}

extern double
statRequestHitMemoryRatio(int minutes)
{
    assert(minutes < N_COUNT_HIST);
    return dpercent(CountHist[0].client_http.mem_hits -
	CountHist[minutes].client_http.mem_hits,
	CountHist[0].client_http.hits -
	CountHist[minutes].client_http.hits);
}

extern double
statRequestHitDiskRatio(int minutes)
{
    assert(minutes < N_COUNT_HIST);
    return dpercent(CountHist[0].client_http.disk_hits -
	CountHist[minutes].client_http.disk_hits,
	CountHist[0].client_http.hits -
	CountHist[minutes].client_http.hits);
}

extern double
statByteHitRatio(int minutes)
{
    size_t s;
    size_t c;
#if USE_CACHE_DIGESTS
    size_t cd;
#endif
    /* size_t might be unsigned */
    assert(minutes < N_COUNT_HIST);
    c = CountHist[0].client_http.kbytes_out.kb - CountHist[minutes].client_http.kbytes_out.kb;
    s = CountHist[0].server.all.kbytes_in.kb - CountHist[minutes].server.all.kbytes_in.kb;
#if USE_CACHE_DIGESTS
    /*
     * This ugly hack is here to prevent the user from seeing a
     * negative byte hit ratio.  When we fetch a cache digest from
     * a neighbor, it gets treated like a cache miss because the
     * object is consumed internally.  Thus, we subtract cache
     * digest bytes out before calculating the byte hit ratio.
     */
    cd = CountHist[0].cd.kbytes_recv.kb - CountHist[minutes].cd.kbytes_recv.kb;
    if (s < cd)
	debug(18, 1) ("STRANGE: srv_kbytes=%d, cd_kbytes=%d\n", (int) s, (int) cd);
    s -= cd;
#endif
    if (c > s)
	return dpercent(c - s, c);
    else
	return (-1.0 * dpercent(s - c, c));
}

static void
statClientRequests(StoreEntry * s)
{
    dlink_node *i;
    clientHttpRequest *http;
    ConnStateData *conn;
    StoreEntry *e;
    int fd;
    for (i = ClientActiveRequests.head; i; i = i->next) {
	http = i->data;
	assert(http);
	conn = http->conn;
	storeAppendPrintf(s, "Connection: %p\n", conn);
	if (conn) {
	    fd = conn->fd;
	    storeAppendPrintf(s, "\tFD %d, read %" PRINTF_OFF_T ", wrote %" PRINTF_OFF_T "\n", fd,
		fd_table[fd].bytes_read, fd_table[fd].bytes_written);
	    storeAppendPrintf(s, "\tFD desc: %s\n", fd_table[fd].desc);
	    storeAppendPrintf(s, "\tin: buf %p, offset %ld, size %ld\n",
		conn->in.buf, (long int) conn->in.offset, (long int) conn->in.size);
	    storeAppendPrintf(s, "\tpeer: %s:%d\n",
		inet_ntoa(conn->peer.sin_addr),
		ntohs(conn->peer.sin_port));
	    storeAppendPrintf(s, "\tme: %s:%d\n",
		inet_ntoa(conn->me.sin_addr),
		ntohs(conn->me.sin_port));
	    storeAppendPrintf(s, "\tnrequests: %d\n",
		conn->nrequests);
	    storeAppendPrintf(s, "\tdefer: n %d, until %ld\n",
		conn->defer.n, (long int) conn->defer.until);
	}
	storeAppendPrintf(s, "uri %s\n", http->uri);
	storeAppendPrintf(s, "log_type %s\n", log_tags[http->log_type]);
	storeAppendPrintf(s, "out.offset %ld, out.size %lu\n",
	    (long int) http->out.offset, (unsigned long int) http->out.size);
	storeAppendPrintf(s, "req_sz %ld\n", (long int) http->req_sz);
	e = http->entry;
	storeAppendPrintf(s, "entry %p/%s\n", e, e ? storeKeyText(e->hash.key) : "N/A");
	e = http->old_entry;
	storeAppendPrintf(s, "old_entry %p/%s\n", e, e ? storeKeyText(e->hash.key) : "N/A");
	storeAppendPrintf(s, "start %ld.%06d (%f seconds ago)\n",
	    (long int) http->start.tv_sec,
	    (int) http->start.tv_usec,
	    tvSubDsec(http->start, current_time));
#if DELAY_POOLS
	storeAppendPrintf(s, "delay_pool %d\n", delayClient(http) >> 16);
#endif
	storeAppendPrintf(s, "\n");
    }
}

#if STAT_GRAPHS
/*
 * urgh, i don't like these, but they do cut the amount of code down immensely
 */

#define GRAPH_PER_MIN(Y) \
    for (i=0;i<(N_COUNT_HIST-2);i++) { \
	dt = tvSubDsec(CountHist[i].timestamp, CountHist[i+1].timestamp); \
	if (dt <= 0.0) \
	    break; \
	storeAppendPrintf(e, "%lu,%0.2f:", \
	    CountHist[i].timestamp.tv_sec, \
	    ((CountHist[i].Y - CountHist[i+1].Y) / dt)); \
    }

#define GRAPH_PER_HOUR(Y) \
    for (i=0;i<(N_COUNT_HOUR_HIST-2);i++) { \
	dt = tvSubDsec(CountHourHist[i].timestamp, CountHourHist[i+1].timestamp); \
	if (dt <= 0.0) \
	    break; \
	storeAppendPrintf(e, "%lu,%0.2f:", \
	    CountHourHist[i].timestamp.tv_sec, \
	    ((CountHourHist[i].Y - CountHourHist[i+1].Y) / dt)); \
    }

#define GRAPH_TITLE(X,Y) storeAppendPrintf(e,"%s\t%s\t",X,Y);
#define GRAPH_END storeAppendPrintf(e,"\n");

#define GENGRAPH(X,Y,Z) \
    GRAPH_TITLE(Y,Z) \
    GRAPH_PER_MIN(X) \
    GRAPH_PER_HOUR(X) \
    GRAPH_END

static void
statGraphDump(StoreEntry * e)
{
    int i;
    double dt;

    GENGRAPH(client_http.requests, "client_http.requests", "Client HTTP requests/sec");
    GENGRAPH(client_http.hits, "client_http.hits", "Client HTTP hits/sec");
    GENGRAPH(client_http.errors, "client_http.errors", "Client HTTP errors/sec");
    GENGRAPH(client_http.kbytes_in.kb, "client_http.kbytes_in", "Client HTTP kbytes_in/sec");
    GENGRAPH(client_http.kbytes_out.kb, "client_http.kbytes_out", "Client HTTP kbytes_out/sec");

    /* XXX todo: http median service times */

    GENGRAPH(server.all.requests, "server.all.requests", "Server requests/sec");
    GENGRAPH(server.all.errors, "server.all.errors", "Server errors/sec");
    GENGRAPH(server.all.kbytes_in.kb, "server.all.kbytes_in", "Server total kbytes_in/sec");
    GENGRAPH(server.all.kbytes_out.kb, "server.all.kbytes_out", "Server total kbytes_out/sec");

    GENGRAPH(server.http.requests, "server.http.requests", "Server HTTP requests/sec");
    GENGRAPH(server.http.errors, "server.http.errors", "Server HTTP errors/sec");
    GENGRAPH(server.http.kbytes_in.kb, "server.http.kbytes_in", "Server HTTP kbytes_in/sec");
    GENGRAPH(server.http.kbytes_out.kb, "server.http.kbytes_out", "Server HTTP kbytes_out/sec");

    GENGRAPH(server.ftp.requests, "server.ftp.requests", "Server FTP requests/sec");
    GENGRAPH(server.ftp.errors, "server.ftp.errors", "Server FTP errors/sec");
    GENGRAPH(server.ftp.kbytes_in.kb, "server.ftp.kbytes_in", "Server FTP kbytes_in/sec");
    GENGRAPH(server.ftp.kbytes_out.kb, "server.ftp.kbytes_out", "Server FTP kbytes_out/sec");

    GENGRAPH(server.other.requests, "server.other.requests", "Server other requests/sec");
    GENGRAPH(server.other.errors, "server.other.errors", "Server other errors/sec");
    GENGRAPH(server.other.kbytes_in.kb, "server.other.kbytes_in", "Server other kbytes_in/sec");
    GENGRAPH(server.other.kbytes_out.kb, "server.other.kbytes_out", "Server other kbytes_out/sec");

    GENGRAPH(icp.pkts_sent, "icp.pkts_sent", "ICP packets sent/sec");
    GENGRAPH(icp.pkts_recv, "icp.pkts_recv", "ICP packets received/sec");
    GENGRAPH(icp.kbytes_sent.kb, "icp.kbytes_sent", "ICP kbytes_sent/sec");
    GENGRAPH(icp.kbytes_recv.kb, "icp.kbytes_recv", "ICP kbytes_received/sec");

    /* XXX todo: icp median service times */
    /* XXX todo: dns median service times */

    GENGRAPH(unlink.requests, "unlink.requests", "Cache File unlink requests/sec");
    GENGRAPH(page_faults, "page_faults", "System Page Faults/sec");
    GENGRAPH(select_loops, "select_loops", "System Select Loop calls/sec");
    GENGRAPH(cputime, "cputime", "CPU utilisation");
}

#endif /* STAT_GRAPHS */

int
statMemoryAccounted(void)
{
    return memTotalAllocated();
}
