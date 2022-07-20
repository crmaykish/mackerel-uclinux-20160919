
/*
 * $Id: store_rebuild.c,v 1.77.2.1 2003/01/09 04:57:41 robertc Exp $
 *
 * DEBUG: section 20    Store Rebuild Routines
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

static struct _store_rebuild_data counts;
static struct timeval rebuild_start;
static void storeCleanup(void *);

typedef struct {
    /* total number of "swap.state" entries that will be read */
    int total;
    /* number of entries read so far */
    int scanned;
} store_rebuild_progress;

static store_rebuild_progress *RebuildProgress = NULL;

static int
storeCleanupDoubleCheck(StoreEntry * e)
{
    SwapDir *SD = &Config.cacheSwap.swapDirs[e->swap_dirn];
    if (SD->dblcheck)
	return (SD->dblcheck(SD, e));
    return 0;
}

static void
storeCleanup(void *datanotused)
{
    static int bucketnum = -1;
    static int validnum = 0;
    static int store_errors = 0;
    int validnum_start;
    StoreEntry *e;
    hash_link *link_ptr = NULL;
    hash_link *link_next = NULL;
    validnum_start = validnum;
    while (validnum - validnum_start < 500) {
	if (++bucketnum >= store_hash_buckets) {
	    debug(20, 1) ("  Completed Validation Procedure\n");
	    debug(20, 1) ("  Validated %d Entries\n", validnum);
	    debug(20, 1) ("  store_swap_size = %dk\n", store_swap_size);
	    store_dirs_rebuilding--;
	    assert(0 == store_dirs_rebuilding);
	    if (opt_store_doublecheck)
		assert(store_errors == 0);
	    if (store_digest)
		storeDigestNoteStoreReady();
	    return;
	}
	link_next = hash_get_bucket(store_table, bucketnum);
	while (NULL != (link_ptr = link_next)) {
	    link_next = link_ptr->next;
	    e = (StoreEntry *) link_ptr;
	    if (EBIT_TEST(e->flags, ENTRY_VALIDATED))
		continue;
	    /*
	     * Calling storeRelease() has no effect because we're
	     * still in 'store_rebuilding' state
	     */
	    if (e->swap_filen < 0)
		continue;
	    if (opt_store_doublecheck)
		if (storeCleanupDoubleCheck(e))
		    store_errors++;
	    EBIT_SET(e->flags, ENTRY_VALIDATED);
	    /*
	     * Only set the file bit if we know its a valid entry
	     * otherwise, set it in the validation procedure
	     */
	    storeDirUpdateSwapSize(&Config.cacheSwap.swapDirs[e->swap_dirn], e->swap_file_sz, 1);
	    if ((++validnum & 0x3FFFF) == 0)
		debug(20, 1) ("  %7d Entries Validated so far.\n", validnum);
	}
    }
    eventAdd("storeCleanup", storeCleanup, NULL, 0.0, 1);
}

/* meta data recreated from disk image in swap directory */
void
storeRebuildComplete(struct _store_rebuild_data *dc)
{
    double dt;
    counts.objcount += dc->objcount;
    counts.expcount += dc->expcount;
    counts.scancount += dc->scancount;
    counts.clashcount += dc->clashcount;
    counts.dupcount += dc->dupcount;
    counts.cancelcount += dc->cancelcount;
    counts.invalid += dc->invalid;
    counts.badflags += dc->badflags;
    counts.bad_log_op += dc->bad_log_op;
    counts.zero_object_sz += dc->zero_object_sz;
    /*
     * When store_dirs_rebuilding == 1, it means we are done reading
     * or scanning all cache_dirs.  Now report the stats and start
     * the validation (storeCleanup()) thread.
     */
    if (store_dirs_rebuilding > 1)
	return;
    dt = tvSubDsec(rebuild_start, current_time);
    debug(20, 1) ("Finished rebuilding storage from disk.\n");
    debug(20, 1) ("  %7d Entries scanned\n", counts.scancount);
    debug(20, 1) ("  %7d Invalid entries.\n", counts.invalid);
    debug(20, 1) ("  %7d With invalid flags.\n", counts.badflags);
    debug(20, 1) ("  %7d Objects loaded.\n", counts.objcount);
    debug(20, 1) ("  %7d Objects expired.\n", counts.expcount);
    debug(20, 1) ("  %7d Objects cancelled.\n", counts.cancelcount);
    debug(20, 1) ("  %7d Duplicate URLs purged.\n", counts.dupcount);
    debug(20, 1) ("  %7d Swapfile clashes avoided.\n", counts.clashcount);
    debug(20, 1) ("  Took %3.1f seconds (%6.1f objects/sec).\n", dt,
	(double) counts.objcount / (dt > 0.0 ? dt : 1.0));
    debug(20, 1) ("Beginning Validation Procedure\n");
    eventAdd("storeCleanup", storeCleanup, NULL, 0.0, 1);
    xfree(RebuildProgress);
    RebuildProgress = NULL;
}

/*
 * this is ugly.  We don't actually start any rebuild threads here,
 * but only initialize counters, etc.  The rebuild threads are
 * actually started by the filesystem "fooDirInit" function.
 */
void
storeRebuildStart(void)
{
    memset(&counts, '\0', sizeof(counts));
    rebuild_start = current_time;
    /*
     * Note: store_dirs_rebuilding is initialized to 1 in globals.c.
     * This prevents us from trying to write clean logs until we
     * finished rebuilding for sure.  The corresponding decrement
     * occurs in storeCleanup(), when it is finished.
     */
    RebuildProgress = xcalloc(Config.cacheSwap.n_configured,
	sizeof(store_rebuild_progress));
}

/*
 * A fs-specific rebuild procedure periodically reports its
 * progress.
 */
void
storeRebuildProgress(int sd_index, int total, int sofar)
{
    static time_t last_report = 0;
    double n = 0.0;
    double d = 0.0;
    if (sd_index < 0)
	return;
    if (sd_index >= Config.cacheSwap.n_configured)
	return;
    if (NULL == RebuildProgress)
	return;
    RebuildProgress[sd_index].total = total;
    RebuildProgress[sd_index].scanned = sofar;
    if (squid_curtime - last_report < 15)
	return;
    for (sd_index = 0; sd_index < Config.cacheSwap.n_configured; sd_index++) {
	n += (double) RebuildProgress[sd_index].scanned;
	d += (double) RebuildProgress[sd_index].total;
    }
    debug(20, 1) ("Store rebuilding is %4.1f%% complete\n", 100.0 * n / d);
    last_report = squid_curtime;
}
