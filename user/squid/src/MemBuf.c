
/*
 * $Id: MemBuf.c,v 1.28.2.4 2005/03/26 02:50:51 hno Exp $
 *
 * DEBUG: section 59    auto-growing Memory Buffer with printf
 * AUTHOR: Alex Rousskov
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

/*
 * To-Do: use memory pools for .buf recycling @?@ @?@
 */

/*
 * Rationale:
 * ----------
 * 
 * Here is how one would comm_write an object without MemBuffer:
 * 
 * {
 * -- allocate:
 * buf = malloc(big_enough);
 * 
 * -- "pack":
 * snprintf object(s) piece-by-piece constantly checking for overflows
 * and maintaining (buf+offset);
 * ...
 * 
 * -- write
 * comm_write(buf, free, ...);
 * }
 * 
 * The whole "packing" idea is quite messy: We are given a buffer of fixed
 * size and we have to check all the time that we still fit. Sounds logical.
 *
 * However, what happens if we have more data? If we are lucky to stop before
 * we overrun any buffers, we still may have garbage (e.g. half of ETag) in
 * the buffer.
 * 
 * MemBuffer:
 * ----------
 * 
 * MemBuffer is a memory-resident buffer with printf()-like interface. It
 * hides all offest handling and overflow checking. Moreover, it has a
 * build-in control that no partial data has been written.
 * 
 * MemBuffer is designed to handle relatively small data. It starts with a
 * small buffer of configurable size to avoid allocating huge buffers all the
 * time.  MemBuffer doubles the buffer when needed. It assert()s that it will
 * not grow larger than a configurable limit. MemBuffer has virtually no
 * overhead (and can even reduce memory consumption) compared to old
 * "packing" approach.
 * 
 * MemBuffer eliminates both "packing" mess and truncated data:
 * 
 * {
 * -- setup
 * MemBuf buf;
 * 
 * -- required init with optional size tuning (see #defines for defaults)
 * memBufInit(&buf, initial-size, absolute-maximum);
 * 
 * -- "pack" (no need to handle offsets or check for overflows)
 * memBufPrintf(&buf, ...);
 * ...
 * 
 * -- write
 * comm_write_mbuf(fd, buf, handler, data);
 *
 * -- *iff* you did not give the buffer away, free it yourself
 * -- memBufClean(&buf);
 * }
 */

#include "squid.h"

#ifdef VA_COPY
#undef VA_COPY
#endif
#if defined HAVE_VA_COPY
#define VA_COPY va_copy
#elif defined HAVE___VA_COPY
#define VA_COPY __va_copy
#endif

/* local constants */

/* default values for buffer sizes, used by memBufDefInit */
#define MEM_BUF_INIT_SIZE   (2*1024)
#define MEM_BUF_MAX_SIZE    (2*1000*1024*1024)


/* local routines */
static void memBufGrow(MemBuf * mb, mb_size_t min_cap);


/* init with defaults */
void
memBufDefInit(MemBuf * mb)
{
    memBufInit(mb, MEM_BUF_INIT_SIZE, MEM_BUF_MAX_SIZE);
}


/* init with specific sizes */
void
memBufInit(MemBuf * mb, mb_size_t szInit, mb_size_t szMax)
{
    assert(mb);
    assert(szInit > 0 && szMax > 0);

    mb->buf = NULL;
    mb->size = 0;
    mb->max_capacity = szMax;
    mb->capacity = 0;
    mb->freefunc = NULL;

    memBufGrow(mb, szInit);
}

/*
 * cleans the mb; last function to call if you do not give .buf away with
 * memBufFreeFunc
 */
void
memBufClean(MemBuf * mb)
{
    assert(mb);
    assert(mb->buf);
    assert(mb->freefunc);	/* not frozen */

    (*mb->freefunc) (mb->buf);	/* free */
    mb->freefunc = NULL;	/* freeze */
    mb->buf = NULL;
    mb->size = mb->capacity = mb->max_capacity = 0;
}

/* cleans the buffer without changing its capacity 
 * if called with a Null buffer, calls memBufDefInit() */
void
memBufReset(MemBuf * mb)
{
    assert(mb);

    if (memBufIsNull(mb)) {
	memBufDefInit(mb);
    } else {
	assert(mb->freefunc);	/* not frozen */
	/* reset */
	memset(mb->buf, 0, mb->capacity);
	mb->size = 0;
    }
}

/* unfortunate hack to test if the buffer has been Init()ialized */
int
memBufIsNull(MemBuf * mb)
{
    assert(mb);
    if (!mb->buf && !mb->max_capacity && !mb->capacity && !mb->size)
	return 1;		/* is null (not initialized) */
    assert(mb->buf && mb->max_capacity && mb->capacity);	/* paranoid */
    return 0;
}


/* calls memcpy, appends exactly size bytes, extends buffer if needed */
void
memBufAppend(MemBuf * mb, const char *buf, int sz)
{
    assert(mb && buf && sz >= 0);
    assert(mb->buf);
    assert(mb->freefunc);	/* not frozen */

    if (sz > 0) {
	if (mb->size + sz > mb->capacity)
	    memBufGrow(mb, mb->size + sz);
	assert(mb->size + sz <= mb->capacity);	/* paranoid */
	xmemcpy(mb->buf + mb->size, buf, sz);
	mb->size += sz;
    }
}

/* calls memBufVPrintf */
#if STDC_HEADERS
void
memBufPrintf(MemBuf * mb, const char *fmt,...)
{
    va_list args;
    va_start(args, fmt);
#else
void
memBufPrintf(va_alist)
     va_dcl
{
    va_list args;
    MemBuf *mb = NULL;
    const char *fmt = NULL;
    mb_size_t sz = 0;
    va_start(args);
    mb = va_arg(args, MemBuf *);
    fmt = va_arg(args, char *);
#endif
    memBufVPrintf(mb, fmt, args);
    va_end(args);
}


/* vprintf for other printf()'s to use; calls vsnprintf, extends buf if needed */
void
memBufVPrintf(MemBuf * mb, const char *fmt, va_list vargs)
{
#if defined VA_COPY
    va_list ap;
#endif
    int sz = 0;
    assert(mb && fmt);
    assert(mb->buf);
    assert(mb->freefunc);	/* not frozen */
    /* assert in Grow should quit first, but we do not want to have a scary infinite loop */
    while (mb->capacity <= mb->max_capacity) {
	mb_size_t free_space = mb->capacity - mb->size;
	/* put as much as we can */

#if defined VA_COPY
	VA_COPY(ap, vargs);	/* Fix of bug 753. The value of vargs is undefined
				 * * after vsnprintf() returns. Make a copy of vargs
				 * * incase we loop around and call vsnprintf() again.
				 */
	sz = vsnprintf(mb->buf + mb->size, free_space, fmt, ap);
	va_end(ap);
#else
	sz = vsnprintf(mb->buf + mb->size, free_space, fmt, vargs);
#endif
	/* check for possible overflow */
	/* snprintf on Linuz returns -1 on overflows */
	/* snprintf on FreeBSD returns at least free_space on overflows */
	if (sz < 0 || sz >= free_space)
	    memBufGrow(mb, mb->capacity + 1);
	else
	    break;
    }
    mb->size += sz;
    /* on Linux and FreeBSD, '\0' is not counted in return value */
    /* on XXX it might be counted */
    /* check that '\0' is appended and not counted */
    if (!mb->size || mb->buf[mb->size - 1]) {
	assert(!mb->buf[mb->size]);
    } else {
	mb->size--;
    }
}

/*
 * returns free() function to be used.
 * Important:
 *   calling this function "freezes" mb,
 *   do not _update_ mb after that in any way
 *   (you still can read-access .buf and .size)
 */
FREE *
memBufFreeFunc(MemBuf * mb)
{
    FREE *ff;
    assert(mb);
    assert(mb->buf);
    assert(mb->freefunc);	/* not frozen */

    ff = mb->freefunc;
    mb->freefunc = NULL;	/* freeze */
    return ff;
}

/* grows (doubles) internal buffer to satisfy required minimal capacity */
static void
memBufGrow(MemBuf * mb, mb_size_t min_cap)
{
    mb_size_t new_cap;
    MemBuf old_mb;

    assert(mb);
    assert(mb->capacity < min_cap);

    /* determine next capacity */
    new_cap = mb->capacity;
    if (new_cap > 0)
	while (new_cap < min_cap)
	    new_cap *= 2;	/* double */
    else
	new_cap = min_cap;

    /* last chance to fit before we assert(!overflow) */
    if (new_cap > mb->max_capacity)
	new_cap = mb->max_capacity;

    assert(new_cap <= mb->max_capacity);	/* no overflow */
    assert(new_cap > mb->capacity);	/* progress */

    old_mb = *mb;

    /* allocate new memory */
    switch (new_cap) {
    case 2048:
	mb->buf = memAllocate(MEM_2K_BUF);
	mb->freefunc = &memFree2K;
	break;
    case 4096:
	mb->buf = memAllocate(MEM_4K_BUF);
	mb->freefunc = &memFree4K;
	break;
    case 8192:
	mb->buf = memAllocate(MEM_8K_BUF);
	mb->freefunc = &memFree8K;
	break;
    case 16384:
	mb->buf = memAllocate(MEM_16K_BUF);
	mb->freefunc = &memFree16K;
	break;
    case 32768:
	mb->buf = memAllocate(MEM_32K_BUF);
	mb->freefunc = &memFree32K;
	break;
    case 65536:
	mb->buf = memAllocate(MEM_64K_BUF);
	mb->freefunc = &memFree64K;
	break;
    default:
	/* recycle if old buffer was not "pool"ed */
	if (old_mb.freefunc == &xfree) {
	    mb->buf = xrealloc(old_mb.buf, new_cap);
	    old_mb.buf = NULL;
	    old_mb.freefunc = NULL;
	    /* init tail, just in case */
	    memset(mb->buf + mb->size, 0, new_cap - mb->size);
	} else {
	    mb->buf = xcalloc(1, new_cap);
	    mb->freefunc = &xfree;
	}
    }

    /* copy and free old buffer if needed */
    if (old_mb.buf && old_mb.freefunc) {
	xmemcpy(mb->buf, old_mb.buf, old_mb.size);
	(*old_mb.freefunc) (old_mb.buf);
    } else {
	assert(!old_mb.buf && !old_mb.freefunc);
    }

    /* done */
    mb->capacity = new_cap;
}


/* Reports */

/* puts report on MemBuf _module_ usage into mb */
void
memBufReport(MemBuf * mb)
{
    assert(mb);
    memBufPrintf(mb, "memBufReport is not yet implemented @?@\n");
}

int
memBufRead(int fd, MemBuf * mb)
{
    int len;
    if (mb->capacity == mb->size)
	memBufGrow(mb, SQUID_TCP_SO_RCVBUF);
    len = FD_READ_METHOD(fd, mb->buf + mb->size, mb->capacity - mb->size);
    if (len)
	mb->size += len;
    return len;
}
