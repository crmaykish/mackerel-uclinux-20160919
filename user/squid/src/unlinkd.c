
/*
 * $Id: unlinkd.c,v 1.44.2.2 2003/07/21 22:34:50 wessels Exp $
 *
 * DEBUG: section 2     Unlink Daemon
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

#ifdef UNLINK_DAEMON

/* This is the external unlinkd process */

#define UNLINK_BUF_LEN 1024

int
main(int argc, char *argv[])
{
    char buf[UNLINK_BUF_LEN];
    char *t;
    int x;
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    close(2);
    open("/dev/null", O_RDWR);
    while (fgets(buf, UNLINK_BUF_LEN, stdin)) {
	if ((t = strchr(buf, '\n')))
	    *t = '\0';
#if USE_TRUNCATE
	x = truncate(buf, 0);
#else
	x = unlink(buf);
#endif
	if (x < 0)
	    printf("ERR\n");
	else
	    printf("OK\n");
    }
    exit(0);
}

#else /* UNLINK_DAEMON */

/* This code gets linked to Squid */

static int unlinkd_wfd = -1;
static int unlinkd_rfd = -1;

#define UNLINKD_QUEUE_LIMIT 20

void
unlinkdUnlink(const char *path)
{
    char buf[MAXPATHLEN];
    int l;
    int x;
    static int queuelen = 0;
    if (unlinkd_wfd < 0) {
	debug_trap("unlinkdUnlink: unlinkd_wfd < 0");
	safeunlink(path, 0);
	return;
    }
    /*
     * If the queue length is greater than our limit, then
     * we pause for up to 100ms, hoping that unlinkd
     * has some feedback for us.  Maybe it just needs a slice
     * of the CPU's time.
     */
    if (queuelen >= UNLINKD_QUEUE_LIMIT) {
	struct timeval to;
	fd_set R;
	FD_ZERO(&R);
	FD_SET(unlinkd_rfd, &R);
	to.tv_sec = 0;
	to.tv_usec = 100000;
	select(unlinkd_rfd + 1, &R, NULL, NULL, &to);
    }
    /*
     * If there is at least one outstanding unlink request, then
     * try to read a response.  If there's nothing to read we'll
     * get an EWOULDBLOCK or whatever.  If we get a response, then
     * decrement the queue size by the number of newlines read.
     */
    if (queuelen > 0) {
	int x;
	int i;
	char rbuf[512];
	x = read(unlinkd_rfd, rbuf, 511);
	if (x > 0) {
	    rbuf[x] = '\0';
	    for (i = 0; i < x; i++)
		if ('\n' == rbuf[i])
		    queuelen--;
	    assert(queuelen >= 0);
	}
    }
    l = strlen(path);
    assert(l < MAXPATHLEN);
    xstrncpy(buf, path, MAXPATHLEN);
    buf[l++] = '\n';
    x = write(unlinkd_wfd, buf, l);
    if (x < 0) {
	debug(2, 1) ("unlinkdUnlink: write FD %d failed: %s\n",
	    unlinkd_wfd, xstrerror());
	safeunlink(path, 0);
	return;
    } else if (x != l) {
	debug(2, 1) ("unlinkdUnlink: FD %d only wrote %d of %d bytes\n",
	    unlinkd_wfd, x, l);
	safeunlink(path, 0);
	return;
    }
    statCounter.unlink.requests++;
    statCounter.syscalls.disk.unlinks++;
    queuelen++;
}

void
unlinkdClose(void)
{
    if (unlinkd_wfd < 0)
	return;
    debug(2, 1) ("Closing unlinkd pipe on FD %d\n", unlinkd_wfd);
    file_close(unlinkd_wfd);
    if (unlinkd_wfd != unlinkd_rfd)
	file_close(unlinkd_rfd);
    unlinkd_wfd = -1;
    unlinkd_rfd = -1;
}

void
unlinkdInit(void)
{
    int x;
    const char *args[2];
    struct timeval slp;
    args[0] = "(unlinkd)";
    args[1] = NULL;
#if HAVE_POLL && defined(_SQUID_OSF_)
    /* pipes and poll() don't get along on DUNIX -DW */
    x = ipcCreate(IPC_TCP_SOCKET,
#else
    x = ipcCreate(IPC_FIFO,
#endif
	Config.Program.unlinkd,
	args,
	"unlinkd",
	&unlinkd_rfd,
	&unlinkd_wfd);
    if (x < 0)
	fatal("Failed to create unlinkd subprocess");
    slp.tv_sec = 0;
    slp.tv_usec = 250000;
    select(0, NULL, NULL, NULL, &slp);
    fd_note(unlinkd_wfd, "squid -> unlinkd");
    fd_note(unlinkd_rfd, "unlinkd -> squid");
    commSetTimeout(unlinkd_rfd, -1, NULL, NULL);
    commSetTimeout(unlinkd_wfd, -1, NULL, NULL);
    /*
     * unlinkd_rfd should already be non-blocking because of
     * ipcCreate.  We change unlinkd_wfd to blocking mode because
     * we never want to lose an unlink request, and we don't have
     * code to retry if we get EWOULDBLOCK.  Unfortunately, we can
     * do this only for the IPC_FIFO case.
     */
    assert(fd_table[unlinkd_rfd].flags.nonblocking);
    if (FD_PIPE == fd_table[unlinkd_wfd].type)
	commUnsetNonBlocking(unlinkd_wfd);
    debug(2, 1) ("Unlinkd pipe opened on FD %d\n", unlinkd_wfd);
}

#endif /* ndef UNLINK_DAEMON */
