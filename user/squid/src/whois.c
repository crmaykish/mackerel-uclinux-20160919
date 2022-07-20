
/*
 * $Id: whois.c,v 1.16 2001/04/14 00:03:24 hno Exp $
 *
 * DEBUG: section 75    WHOIS protocol
 * AUTHOR: Duane Wessels, Kostas Anagnostakis
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

#define WHOIS_PORT 43

typedef struct {
    StoreEntry *entry;
    request_t *request;
    FwdState *fwd;
} WhoisState;

static PF whoisClose;
static PF whoisTimeout;
static PF whoisReadReply;

/* PUBLIC */

CBDATA_TYPE(WhoisState);

void
whoisStart(FwdState * fwd)
{
    WhoisState *p;
    int fd = fwd->server_fd;
    char *buf;
    size_t l;
    CBDATA_INIT_TYPE(WhoisState);
    p = cbdataAlloc(WhoisState);
    p->request = fwd->request;
    p->entry = fwd->entry;
    p->fwd = fwd;
    storeLockObject(p->entry);
    comm_add_close_handler(fd, whoisClose, p);
    l = strLen(p->request->urlpath) + 3;
    buf = xmalloc(l);
    snprintf(buf, l, "%s\r\n", strBuf(p->request->urlpath) + 1);
    comm_write(fd, buf, strlen(buf), NULL, p, xfree);
    commSetSelect(fd, COMM_SELECT_READ, whoisReadReply, p, 0);
    commSetTimeout(fd, Config.Timeout.read, whoisTimeout, p);
}

/* PRIVATE */

static void
whoisTimeout(int fd, void *data)
{
    WhoisState *p = data;
    debug(75, 1) ("whoisTimeout: %s\n", storeUrl(p->entry));
    whoisClose(fd, p);
}

static void
whoisReadReply(int fd, void *data)
{
    WhoisState *p = data;
    StoreEntry *entry = p->entry;
    char *buf = memAllocate(MEM_4K_BUF);
    MemObject *mem = entry->mem_obj;
    int len;
    statCounter.syscalls.sock.reads++;
    len = FD_READ_METHOD(fd, buf, 4095);
    buf[len] = '\0';
    debug(75, 3) ("whoisReadReply: FD %d read %d bytes\n", fd, len);
    debug(75, 5) ("{%s}\n", buf);
    if (len > 0) {
	if (0 == mem->inmem_hi)
	    mem->reply->sline.status = HTTP_OK;
	fd_bytes(fd, len, FD_READ);
	kb_incr(&statCounter.server.all.kbytes_in, len);
	kb_incr(&statCounter.server.http.kbytes_in, len);
	storeAppend(entry, buf, len);
	commSetSelect(fd, COMM_SELECT_READ, whoisReadReply, p, Config.Timeout.read);
    } else if (len < 0) {
	debug(50, 2) ("whoisReadReply: FD %d: read failure: %s.\n",
	    fd, xstrerror());
	if (ignoreErrno(errno)) {
	    commSetSelect(fd, COMM_SELECT_READ, whoisReadReply, p, Config.Timeout.read);
	} else if (mem->inmem_hi == 0) {
	    ErrorState *err;
	    err = errorCon(ERR_READ_ERROR, HTTP_INTERNAL_SERVER_ERROR);
	    err->xerrno = errno;
	    fwdFail(p->fwd, err);
	    comm_close(fd);
	} else {
	    comm_close(fd);
	}
    } else {
	fwdComplete(p->fwd);
	debug(75, 3) ("whoisReadReply: Done: %s\n", storeUrl(entry));
	comm_close(fd);
    }
    memFree(buf, MEM_4K_BUF);
}

static void
whoisClose(int fd, void *data)
{
    WhoisState *p = data;
    debug(75, 3) ("whoisClose: FD %d\n", fd);
    storeUnlockObject(p->entry);
    cbdataFree(p);
}
