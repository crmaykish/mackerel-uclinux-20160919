
/*
 * $Id: store_dir.c,v 1.135.2.3 2005/03/26 02:50:54 hno Exp $
 *
 * DEBUG: section 47    Store Directory Routines
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

#if HAVE_STATVFS
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#endif
/* Windows uses sys/vfs.h */
#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

static int storeDirValidSwapDirSize(int, squid_off_t);
static STDIRSELECT storeDirSelectSwapDirRoundRobin;
static STDIRSELECT storeDirSelectSwapDirLeastLoad;

/*
 * This function pointer is set according to 'store_dir_select_algorithm'
 * in squid.conf.
 */
STDIRSELECT *storeDirSelectSwapDir = storeDirSelectSwapDirLeastLoad;

void
storeDirInit(void)
{
    int i;
    SwapDir *sd;
    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	sd = &Config.cacheSwap.swapDirs[i];
	sd->init(sd);
    }
    if (0 == strcasecmp(Config.store_dir_select_algorithm, "round-robin")) {
	storeDirSelectSwapDir = storeDirSelectSwapDirRoundRobin;
	debug(47, 1) ("Using Round Robin store dir selection\n");
    } else {
	storeDirSelectSwapDir = storeDirSelectSwapDirLeastLoad;
	debug(47, 1) ("Using Least Load store dir selection\n");
    }
}

void
storeCreateSwapDirectories(void)
{
    int i;
    SwapDir *sd;
    pid_t pid;
    int status;
    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	if (fork())
	    continue;
	sd = &Config.cacheSwap.swapDirs[i];
	if (NULL != sd->newfs)
	    sd->newfs(sd);
	exit(0);
    }
    do {
#ifdef _SQUID_NEXT_
	pid = wait3(&status, WNOHANG, NULL);
#else
	pid = waitpid(-1, &status, 0);
#endif
    } while (pid > 0 || (pid < 0 && errno == EINTR));
}

/*
 * Determine whether the given directory can handle this object
 * size
 *
 * Note: if the object size is -1, then the only swapdirs that
 * will return true here are ones that have max_obj_size = -1,
 * ie any-sized-object swapdirs. This is a good thing.
 */
static int
storeDirValidSwapDirSize(int swapdir, squid_off_t objsize)
{
    /*
     * If the swapdir's max_obj_size is -1, then it definitely can
     */
    if (Config.cacheSwap.swapDirs[swapdir].max_objsize == -1)
	return 1;

    /*
     * If the object size is -1, then if the storedir isn't -1 we
     * can't store it
     */
    if ((objsize == -1) &&
	(Config.cacheSwap.swapDirs[swapdir].max_objsize != -1))
	return 0;

    /*
     * Else, make sure that the max object size is larger than objsize
     */
    if (Config.cacheSwap.swapDirs[swapdir].max_objsize > objsize)
	return 1;
    else
	return 0;
}


/*
 * This new selection scheme simply does round-robin on all SwapDirs.
 * A SwapDir is skipped if it is over the max_size (100%) limit, or
 * overloaded.
 */
static int
storeDirSelectSwapDirRoundRobin(const StoreEntry * e)
{
    static int dirn = 0;
    int i;
    int load;
    SwapDir *sd;
    squid_off_t objsize = objectLen(e);
    for (i = 0; i <= Config.cacheSwap.n_configured; i++) {
	if (++dirn >= Config.cacheSwap.n_configured)
	    dirn = 0;
	sd = &Config.cacheSwap.swapDirs[dirn];
	if (sd->flags.read_only)
	    continue;
	if (sd->cur_size > sd->max_size)
	    continue;
	if (!storeDirValidSwapDirSize(dirn, objsize))
	    continue;
	/* check for error or overload condition */
	load = sd->checkobj(sd, e);
	if (load < 0 || load > 1000) {
	    continue;
	}
	return dirn;
    }
    return -1;
}

/*
 * Spread load across all of the store directories
 *
 * Note: We should modify this later on to prefer sticking objects
 * in the *tightest fit* swapdir to conserve space, along with the
 * actual swapdir usage. But for now, this hack will do while  
 * testing, so you should order your swapdirs in the config file
 * from smallest maxobjsize to unlimited (-1) maxobjsize.
 *
 * We also have to choose nleast == nconf since we need to consider
 * ALL swapdirs, regardless of state. Again, this is a hack while
 * we sort out the real usefulness of this algorithm.
 */
static int
storeDirSelectSwapDirLeastLoad(const StoreEntry * e)
{
    squid_off_t objsize;
    int most_free = 0, cur_free;
    squid_off_t least_objsize = -1;
    int least_load = INT_MAX;
    int load;
    int dirn = -1;
    int i;
    SwapDir *SD;

    /* Calculate the object size */
    objsize = objectLen(e);
    if (objsize != -1)
	objsize += e->mem_obj->swap_hdr_sz;
    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	SD = &Config.cacheSwap.swapDirs[i];
	SD->flags.selected = 0;
	load = SD->checkobj(SD, e);
	if (load < 0 || load > 1000) {
	    continue;
	}
	if (!storeDirValidSwapDirSize(i, objsize))
	    continue;
	if (SD->flags.read_only)
	    continue;
	if (SD->cur_size > SD->max_size)
	    continue;
	if (load > least_load)
	    continue;
	cur_free = SD->max_size - SD->cur_size;
	/* If the load is equal, then look in more details */
	if (load == least_load) {
	    /* closest max_objsize fit */
	    if (least_objsize != -1)
		if (SD->max_objsize > least_objsize || SD->max_objsize == -1)
		    continue;
	    /* most free */
	    if (cur_free < most_free)
		continue;
	}
	least_load = load;
	least_objsize = SD->max_objsize;
	most_free = cur_free;
	dirn = i;
    }
    if (dirn >= 0)
	Config.cacheSwap.swapDirs[dirn].flags.selected = 1;
    return dirn;
}



char *
storeSwapDir(int dirn)
{
    assert(0 <= dirn && dirn < Config.cacheSwap.n_configured);
    return Config.cacheSwap.swapDirs[dirn].path;
}

/*
 * An entry written to the swap log MUST have the following
 * properties.
 *   1.  It MUST be a public key.  It does no good to log
 *       a public ADD, change the key, then log a private
 *       DEL.  So we need to log a DEL before we change a
 *       key from public to private.
 *   2.  It MUST have a valid (> -1) swap_filen.
 */
void
storeDirSwapLog(const StoreEntry * e, int op)
{
    SwapDir *sd;
    assert(!EBIT_TEST(e->flags, KEY_PRIVATE));
    assert(e->swap_filen >= 0);
    /*
     * icons and such; don't write them to the swap log
     */
    if (EBIT_TEST(e->flags, ENTRY_SPECIAL))
	return;
    assert(op > SWAP_LOG_NOP && op < SWAP_LOG_MAX);
    debug(20, 3) ("storeDirSwapLog: %s %s %d %08X\n",
	swap_log_op_str[op],
	storeKeyText(e->hash.key),
	e->swap_dirn,
	e->swap_filen);
    sd = &Config.cacheSwap.swapDirs[e->swap_dirn];
    sd->log.write(sd, e, op);
}

void
storeDirUpdateSwapSize(SwapDir * SD, squid_off_t size, int sign)
{
    int blks = (size + SD->fs.blksize - 1) / SD->fs.blksize;
    int k = (blks * SD->fs.blksize >> 10) * sign;
    SD->cur_size += k;
    store_swap_size += k;
    if (sign > 0)
	n_disk_objects++;
    else if (sign < 0)
	n_disk_objects--;
}

void
storeDirStats(StoreEntry * sentry)
{
    int i;
    SwapDir *SD;

    storeAppendPrintf(sentry, "Store Directory Statistics:\n");
    storeAppendPrintf(sentry, "Store Entries          : %d\n",
	memInUse(MEM_STOREENTRY));
    storeAppendPrintf(sentry, "Maximum Swap Size      : %8ld KB\n",
	(long int) Config.Swap.maxSize);
    storeAppendPrintf(sentry, "Current Store Swap Size: %8d KB\n",
	store_swap_size);
    storeAppendPrintf(sentry, "Current Capacity       : %d%% used, %d%% free\n",
	percent((int) store_swap_size, (int) Config.Swap.maxSize),
	percent((int) (Config.Swap.maxSize - store_swap_size), (int) Config.Swap.maxSize));
    /* FIXME Here we should output memory statistics */

    /* Now go through each swapdir, calling its statfs routine */
    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	storeAppendPrintf(sentry, "\n");
	SD = &(Config.cacheSwap.swapDirs[i]);
	storeAppendPrintf(sentry, "Store Directory #%d (%s): %s\n", i, SD->type,
	    storeSwapDir(i));
	storeAppendPrintf(sentry, "FS Block Size %d Bytes\n",
	    SD->fs.blksize);
	SD->statfs(SD, sentry);
	if (SD->repl) {
	    storeAppendPrintf(sentry, "Removal policy: %s\n", SD->repl->_type);
	    if (SD->repl->Stats)
		SD->repl->Stats(SD->repl, sentry);
	}
    }
}

void
storeDirConfigure(void)
{
    SwapDir *SD;
    int i;
    Config.Swap.maxSize = 0;
    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	SD = &Config.cacheSwap.swapDirs[i];
	Config.Swap.maxSize += SD->max_size;
	SD->low_size = (int) (((float) SD->max_size *
		(float) Config.Swap.lowWaterMark) / 100.0);
    }
}

void
storeDirDiskFull(sdirno dirn)
{
    SwapDir *SD = &Config.cacheSwap.swapDirs[dirn];
    assert(0 <= dirn && dirn < Config.cacheSwap.n_configured);
    if (SD->cur_size >= SD->max_size)
	return;
    SD->max_size = SD->cur_size;
    debug(20, 1) ("WARNING: Shrinking cache_dir #%d to %d KB\n",
	dirn, SD->cur_size);
}

void
storeDirOpenSwapLogs(void)
{
    int dirn;
    SwapDir *sd;
    for (dirn = 0; dirn < Config.cacheSwap.n_configured; dirn++) {
	sd = &Config.cacheSwap.swapDirs[dirn];
	if (sd->log.open)
	    sd->log.open(sd);
    }
}

void
storeDirCloseSwapLogs(void)
{
    int dirn;
    SwapDir *sd;
    for (dirn = 0; dirn < Config.cacheSwap.n_configured; dirn++) {
	sd = &Config.cacheSwap.swapDirs[dirn];
	if (sd->log.close)
	    sd->log.close(sd);
    }
}

/*
 *  storeDirWriteCleanLogs
 * 
 *  Writes a "clean" swap log file from in-memory metadata.
 *  This is a rewrite of the original function to troll each
 *  StoreDir and write the logs, and flush at the end of
 *  the run. Thanks goes to Eric Stern, since this solution
 *  came out of his COSS code.
 */
#define CLEAN_BUF_SZ 16384
int
storeDirWriteCleanLogs(int reopen)
{
    const StoreEntry *e = NULL;
    int n = 0;
    struct timeval start;
    double dt;
    SwapDir *sd;
    int dirn;
    int notdone = 1;
    if (store_dirs_rebuilding) {
	debug(20, 1) ("Not currently OK to rewrite swap log.\n");
	debug(20, 1) ("storeDirWriteCleanLogs: Operation aborted.\n");
	return 0;
    }
    debug(20, 1) ("storeDirWriteCleanLogs: Starting...\n");
    getCurrentTime();
    start = current_time;
    for (dirn = 0; dirn < Config.cacheSwap.n_configured; dirn++) {
	sd = &Config.cacheSwap.swapDirs[dirn];
	if (sd->log.clean.start(sd) < 0) {
	    debug(20, 1) ("log.clean.start() failed for dir #%d\n", sd->index);
	    continue;
	}
    }
    while (notdone) {
	notdone = 0;
	for (dirn = 0; dirn < Config.cacheSwap.n_configured; dirn++) {
	    sd = &Config.cacheSwap.swapDirs[dirn];
	    if (NULL == sd->log.clean.write)
		continue;
	    e = sd->log.clean.nextentry(sd);
	    if (!e)
		continue;
	    notdone = 1;
	    if (e->swap_filen < 0)
		continue;
	    if (e->swap_status != SWAPOUT_DONE)
		continue;
	    if (e->swap_file_sz <= 0)
		continue;
	    if (EBIT_TEST(e->flags, RELEASE_REQUEST))
		continue;
	    if (EBIT_TEST(e->flags, KEY_PRIVATE))
		continue;
	    if (EBIT_TEST(e->flags, ENTRY_SPECIAL))
		continue;
	    sd->log.clean.write(sd, e);
	    if ((++n & 0xFFFF) == 0) {
		getCurrentTime();
		debug(20, 1) ("  %7d entries written so far.\n", n);
	    }
	}
    }
    /* Flush */
    for (dirn = 0; dirn < Config.cacheSwap.n_configured; dirn++) {
	sd = &Config.cacheSwap.swapDirs[dirn];
	sd->log.clean.done(sd);
    }
    if (reopen)
	storeDirOpenSwapLogs();
    getCurrentTime();
    dt = tvSubDsec(start, current_time);
    debug(20, 1) ("  Finished.  Wrote %d entries.\n", n);
    debug(20, 1) ("  Took %3.1f seconds (%6.1f entries/sec).\n",
	dt, (double) n / (dt > 0.0 ? dt : 1.0));
    return n;
}
#undef CLEAN_BUF_SZ

/*
 * sync all avaliable fs'es ..
 */
void
storeDirSync(void)
{
    int i;
    SwapDir *SD;

    for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	SD = &Config.cacheSwap.swapDirs[i];
	if (SD->sync != NULL)
	    SD->sync(SD);
    }
}

/*
 * handle callbacks all avaliable fs'es ..
 */
void
storeDirCallback(void)
{
    int i, j;
    SwapDir *SD;
    static int ndir = 0;
    do {
	j = 0;
	for (i = 0; i < Config.cacheSwap.n_configured; i++) {
	    if (ndir >= Config.cacheSwap.n_configured)
		ndir = ndir % Config.cacheSwap.n_configured;
	    SD = &Config.cacheSwap.swapDirs[ndir++];
	    if (NULL == SD->callback)
		continue;
	    j += SD->callback(SD);
	}
    } while (j > 0);
    ndir++;
}

int
storeDirGetBlkSize(const char *path, int *blksize)
{
#if HAVE_STATVFS
    struct statvfs sfs;
    if (statvfs(path, &sfs)) {
	debug(50, 1) ("%s: %s\n", path, xstrerror());
	*blksize = 2048;
	return 1;
    }
    *blksize = (int) sfs.f_frsize;
#else
    struct statfs sfs;
    if (statfs(path, &sfs)) {
	debug(50, 1) ("%s: %s\n", path, xstrerror());
	*blksize = 2048;
	return 1;
    }
    *blksize = (int) sfs.f_bsize;
#endif
    /*
     * Sanity check; make sure we have a meaningful value.
     */
    if (*blksize < 512)
	*blksize = 2048;
    return 0;
}

#define fsbtoblk(num, fsbs, bs) \
    (((fsbs) != 0 && (fsbs) < (bs)) ? \
            (num) / ((bs) / (fsbs)) : (num) * ((fsbs) / (bs)))
int
storeDirGetUFSStats(const char *path, int *totl_kb, int *free_kb, int *totl_in, int *free_in)
{
#if HAVE_STATVFS
    struct statvfs sfs;
    if (statvfs(path, &sfs)) {
	debug(50, 1) ("%s: %s\n", path, xstrerror());
	return 1;
    }
    *totl_kb = (int) fsbtoblk(sfs.f_blocks, sfs.f_frsize, 1024);
    *free_kb = (int) fsbtoblk(sfs.f_bfree, sfs.f_frsize, 1024);
    *totl_in = (int) sfs.f_files;
    *free_in = (int) sfs.f_ffree;
#else
    struct statfs sfs;
    if (statfs(path, &sfs)) {
	debug(50, 1) ("%s: %s\n", path, xstrerror());
	return 1;
    }
    *totl_kb = (int) fsbtoblk(sfs.f_blocks, sfs.f_bsize, 1024);
    *free_kb = (int) fsbtoblk(sfs.f_bfree, sfs.f_bsize, 1024);
    *totl_in = (int) sfs.f_files;
    *free_in = (int) sfs.f_ffree;
#endif
    return 0;
}
