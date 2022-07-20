
/*
 * $Id: peer_digest.c,v 1.83.2.2 2005/03/26 02:50:53 hno Exp $
 *
 * DEBUG: section 72    Peer Digest Routines
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

#include "squid.h"

#if USE_CACHE_DIGESTS

/* local types */

/* local prototypes */
static time_t peerDigestIncDelay(const PeerDigest * pd);
static time_t peerDigestNewDelay(const StoreEntry * e);
static void peerDigestSetCheck(PeerDigest * pd, time_t delay);
static void peerDigestClean(PeerDigest *);
static EVH peerDigestCheck;
static void peerDigestRequest(PeerDigest * pd);
static STCB peerDigestFetchReply;
static STCB peerDigestSwapInHeaders;
static STCB peerDigestSwapInCBlock;
static STCB peerDigestSwapInMask;
static int peerDigestFetchedEnough(DigestFetchState * fetch, char *buf, ssize_t size, const char *step_name);
static void peerDigestFetchStop(DigestFetchState * fetch, char *buf, const char *reason);
static void peerDigestFetchAbort(DigestFetchState * fetch, char *buf, const char *reason);
static void peerDigestReqFinish(DigestFetchState * fetch, char *buf, int, int, int, const char *reason, int err);
static void peerDigestPDFinish(DigestFetchState * fetch, int pcb_valid, int err);
static void peerDigestFetchFinish(DigestFetchState * fetch, int err);
static void peerDigestFetchSetStats(DigestFetchState * fetch);
static int peerDigestSetCBlock(PeerDigest * pd, const char *buf);
static int peerDigestUseful(const PeerDigest * pd);


/* local constants */

#define StoreDigestCBlockSize sizeof(StoreDigestCBlock)

/* min interval for requesting digests from a given peer */
static const time_t PeerDigestReqMinGap = 5 * 60;	/* seconds */
/* min interval for requesting digests (cumulative request stream) */
static const time_t GlobDigestReqMinGap = 1 * 60;	/* seconds */

/* local vars */

static time_t pd_last_req_time = 0;	/* last call to Check */

/* initialize peer digest */
static void
peerDigestInit(PeerDigest * pd, peer * p)
{
    assert(pd && p);

    memset(pd, 0, sizeof(*pd));
    pd->peer = p;
    /* if peer disappears, we will know it's name */
    stringInit(&pd->host, p->host);

    pd->times.initialized = squid_curtime;
}

static void
peerDigestClean(PeerDigest * pd)
{
    assert(pd);
    if (pd->cd)
	cacheDigestDestroy(pd->cd);
    stringClean(&pd->host);
}

CBDATA_TYPE(PeerDigest);

/* allocate new peer digest, call Init, and lock everything */
PeerDigest *
peerDigestCreate(peer * p)
{
    PeerDigest *pd;
    assert(p);

    CBDATA_INIT_TYPE(PeerDigest);
    pd = cbdataAlloc(PeerDigest);
    peerDigestInit(pd, p);
    cbdataLock(pd->peer);	/* we will use the peer */

    return pd;
}

/* call Clean and free/unlock everything */
static void
peerDigestDestroy(PeerDigest * pd)
{
    peer *p;
    assert(pd);

    p = pd->peer;
    pd->peer = NULL;
    /* inform peer (if any) that we are gone */
    if (cbdataValid(p))
	peerNoteDigestGone(p);
    cbdataUnlock(p);		/* must unlock, valid or not */

    peerDigestClean(pd);
    cbdataFree(pd);
}

/* called by peer to indicate that somebody actually needs this digest */
void
peerDigestNeeded(PeerDigest * pd)
{
    assert(pd);
    assert(!pd->flags.needed);
    assert(!pd->cd);

    pd->flags.needed = 1;
    pd->times.needed = squid_curtime;
    peerDigestSetCheck(pd, 0);	/* check asap */
}

/* currently we do not have a reason to disable without destroying */
#if FUTURE_CODE
/* disables peer for good */
static void
peerDigestDisable(PeerDigest * pd)
{
    debug(72, 2) ("peerDigestDisable: peer %s disabled for good\n",
	strBuf(pd->host));
    pd->times.disabled = squid_curtime;
    pd->times.next_check = -1;	/* never */
    pd->flags.usable = 0;

    if (pd->cd) {
	cacheDigestDestroy(pd->cd);
	pd->cd = NULL;
    }
    /* we do not destroy the pd itself to preserve its "history" and stats */
}
#endif

/* increment retry delay [after an unsuccessful attempt] */
static time_t
peerDigestIncDelay(const PeerDigest * pd)
{
    assert(pd);
    return pd->times.retry_delay > 0 ?
	2 * pd->times.retry_delay :	/* exponential backoff */
	PeerDigestReqMinGap;	/* minimal delay */
}

/* artificially increases Expires: setting to avoid race conditions 
 * returns the delay till that [increased] expiration time */
static time_t
peerDigestNewDelay(const StoreEntry * e)
{
    assert(e);
    if (e->expires > 0)
	return e->expires + PeerDigestReqMinGap - squid_curtime;
    return PeerDigestReqMinGap;
}

/* registers next digest verification */
static void
peerDigestSetCheck(PeerDigest * pd, time_t delay)
{
    eventAdd("peerDigestCheck", peerDigestCheck, pd, (double) delay, 1);
    pd->times.next_check = squid_curtime + delay;
    debug(72, 3) ("peerDigestSetCheck: will check peer %s in %d secs\n",
	strBuf(pd->host), (int) delay);
}

/*
 * called when peer is about to disappear or have already disappeared
 */
void
peerDigestNotePeerGone(PeerDigest * pd)
{
    if (pd->flags.requested) {
	debug(72, 2) ("peerDigest: peer %s gone, will destroy after fetch.\n",
	    strBuf(pd->host));
	/* do nothing now, the fetching chain will notice and take action */
    } else {
	debug(72, 2) ("peerDigest: peer %s is gone, destroying now.\n",
	    strBuf(pd->host));
	peerDigestDestroy(pd);
    }
}

/* callback for eventAdd() (with peer digest locked)
 * request new digest if our copy is too old or if we lack one; 
 * schedule next check otherwise */
static void
peerDigestCheck(void *data)
{
    PeerDigest *pd = data;
    time_t req_time;

    /*
     * you can't assert(cbdataValid(pd)) -- if its not valid this
     * function never gets called
     */
    assert(!pd->flags.requested);

    pd->times.next_check = 0;	/* unknown */

    if (!cbdataValid(pd->peer)) {
	peerDigestNotePeerGone(pd);
	return;
    }
    debug(72, 3) ("peerDigestCheck: peer %s:%d\n", pd->peer->host, pd->peer->http_port);
    debug(72, 3) ("peerDigestCheck: time: %ld, last received: %ld (%+d)\n",
	(long int) squid_curtime, (long int) pd->times.received, (int) (squid_curtime - pd->times.received));

    /* decide when we should send the request:
     * request now unless too close to other requests */
    req_time = squid_curtime;

    /* per-peer limit */
    if (req_time - pd->times.received < PeerDigestReqMinGap) {
	debug(72, 2) ("peerDigestCheck: %s, avoiding close peer requests (%d < %d secs).\n",
	    strBuf(pd->host), (int) (req_time - pd->times.received),
	    (int) PeerDigestReqMinGap);
	req_time = pd->times.received + PeerDigestReqMinGap;
    }
    /* global limit */
    if (req_time - pd_last_req_time < GlobDigestReqMinGap) {
	debug(72, 2) ("peerDigestCheck: %s, avoiding close requests (%d < %d secs).\n",
	    strBuf(pd->host), (int) (req_time - pd_last_req_time),
	    (int) GlobDigestReqMinGap);
	req_time = pd_last_req_time + GlobDigestReqMinGap;
    }
    if (req_time <= squid_curtime)
	peerDigestRequest(pd);	/* will set pd->flags.requested */
    else
	peerDigestSetCheck(pd, req_time - squid_curtime);
}

CBDATA_TYPE(DigestFetchState);

/* ask store for a digest */
static void
peerDigestRequest(PeerDigest * pd)
{
    peer *p = pd->peer;
    StoreEntry *e, *old_e;
    char *url;
    const cache_key *key;
    request_t *req;
    DigestFetchState *fetch = NULL;

    pd->req_result = NULL;
    pd->flags.requested = 1;

    /* compute future request components */
    if (p->digest_url)
	url = xstrdup(p->digest_url);
    else
	url = internalRemoteUri(p->host, p->http_port,
	    "/squid-internal-periodic/", StoreDigestFileName);

    req = urlParse(METHOD_GET, url);
    assert(req);
    key = storeKeyPublicByRequest(req);
    debug(72, 2) ("peerDigestRequest: %s key: %s\n", url, storeKeyText(key));

    /* add custom headers */
    assert(!req->header.len);
    httpHeaderPutStr(&req->header, HDR_ACCEPT, StoreDigestMimeStr);
    httpHeaderPutStr(&req->header, HDR_ACCEPT, "text/html");
    if (p->login)
	xstrncpy(req->login, p->login, MAX_LOGIN_SZ);
    /* create fetch state structure */
    CBDATA_INIT_TYPE(DigestFetchState);
    fetch = cbdataAlloc(DigestFetchState);
    fetch->request = requestLink(req);
    fetch->pd = pd;
    fetch->offset = 0;

    /* update timestamps */
    fetch->start_time = squid_curtime;
    pd->times.requested = squid_curtime;
    pd_last_req_time = squid_curtime;

    req->flags.cachable = 1;
    /* the rest is based on clientProcessExpired() */
    req->flags.refresh = 1;
    old_e = fetch->old_entry = storeGet(key);
    if (old_e) {
	debug(72, 5) ("peerDigestRequest: found old entry\n");
	storeLockObject(old_e);
	storeCreateMemObject(old_e, url, url);
	fetch->old_sc = storeClientListAdd(old_e, fetch);
    }
    e = fetch->entry = storeCreateEntry(url, url, req->flags, req->method);
    assert(EBIT_TEST(e->flags, KEY_PRIVATE));
    fetch->sc = storeClientListAdd(e, fetch);
    /* set lastmod to trigger IMS request if possible */
    if (old_e)
	e->lastmod = old_e->lastmod;

    /* push towards peer cache */
    debug(72, 3) ("peerDigestRequest: forwarding to fwdStart...\n");
    fwdStart(-1, e, req);
    cbdataLock(fetch);
    cbdataLock(fetch->pd);
    storeClientCopy(fetch->sc, e, 0, 0, 4096, memAllocate(MEM_4K_BUF),
	peerDigestFetchReply, fetch);
}

/* wait for full http headers to be received then parse them */
static void
peerDigestFetchReply(void *data, char *buf, ssize_t size)
{
    DigestFetchState *fetch = data;
    PeerDigest *pd = fetch->pd;
    size_t hdr_size;
    assert(pd && buf);
    assert(!fetch->offset);

    if (peerDigestFetchedEnough(fetch, buf, size, "peerDigestFetchReply"))
	return;

    if ((hdr_size = headersEnd(buf, size))) {
	http_status status;
	HttpReply *reply = fetch->entry->mem_obj->reply;
	assert(reply);
	httpReplyParse(reply, buf, hdr_size);
	status = reply->sline.status;
	debug(72, 3) ("peerDigestFetchReply: %s status: %d, expires: %ld (%+d)\n",
	    strBuf(pd->host), status,
	    (long int) reply->expires, (int) (reply->expires - squid_curtime));

	/* this "if" is based on clientHandleIMSReply() */
	if (status == HTTP_NOT_MODIFIED) {
	    request_t *r = NULL;
	    /* our old entry is fine */
	    assert(fetch->old_entry);
	    if (!fetch->old_entry->mem_obj->request)
		fetch->old_entry->mem_obj->request = r =
		    requestLink(fetch->entry->mem_obj->request);
	    assert(fetch->old_entry->mem_obj->request);
	    httpReplyUpdateOnNotModified(fetch->old_entry->mem_obj->reply, reply);
	    storeTimestampsSet(fetch->old_entry);
	    /* get rid of 304 reply */
	    storeUnregister(fetch->sc, fetch->entry, fetch);
	    storeUnlockObject(fetch->entry);
	    fetch->entry = fetch->old_entry;
	    fetch->old_entry = NULL;
	    /* preserve request -- we need its size to update counters */
	    /* requestUnlink(r); */
	    /* fetch->entry->mem_obj->request = NULL; */
	} else if (status == HTTP_OK) {
	    /* get rid of old entry if any */
	    if (fetch->old_entry) {
		debug(72, 3) ("peerDigestFetchReply: got new digest, releasing old one\n");
		storeUnregister(fetch->old_sc, fetch->old_entry, fetch);
		storeReleaseRequest(fetch->old_entry);
		storeUnlockObject(fetch->old_entry);
		fetch->old_entry = NULL;
	    }
	} else {
	    /* some kind of a bug */
	    peerDigestFetchAbort(fetch, buf, httpStatusLineReason(&reply->sline));
	    return;
	}
	/* must have a ready-to-use store entry if we got here */
	/* can we stay with the old in-memory digest? */
	if (status == HTTP_NOT_MODIFIED && fetch->pd->cd)
	    peerDigestFetchStop(fetch, buf, "Not modified");
	else
	    storeClientCopy(fetch->sc, fetch->entry,	/* have to swap in */
		0, 0, SM_PAGE_SIZE, buf, peerDigestSwapInHeaders, fetch);
    } else {
	/* need more data, do we have space? */
	if (size >= SM_PAGE_SIZE)
	    peerDigestFetchAbort(fetch, buf, "reply header too big");
	else
	    storeClientCopy(fetch->sc, fetch->entry, size, 0, SM_PAGE_SIZE, buf,
		peerDigestFetchReply, fetch);
    }
}

/* fetch headers from disk, pass on to SwapInCBlock */
static void
peerDigestSwapInHeaders(void *data, char *buf, ssize_t size)
{
    DigestFetchState *fetch = data;
    size_t hdr_size;

    if (peerDigestFetchedEnough(fetch, buf, size, "peerDigestSwapInHeaders"))
	return;

    assert(!fetch->offset);
    if ((hdr_size = headersEnd(buf, size))) {
	assert(fetch->entry->mem_obj->reply);
	if (!fetch->entry->mem_obj->reply->sline.status)
	    httpReplyParse(fetch->entry->mem_obj->reply, buf, hdr_size);
	if (fetch->entry->mem_obj->reply->sline.status != HTTP_OK) {
	    debug(72, 1) ("peerDigestSwapInHeaders: %s status %d got cached!\n",
		strBuf(fetch->pd->host), fetch->entry->mem_obj->reply->sline.status);
	    peerDigestFetchAbort(fetch, buf, "internal status error");
	    return;
	}
	fetch->offset += hdr_size;
	storeClientCopy(fetch->sc, fetch->entry, size, fetch->offset,
	    SM_PAGE_SIZE, buf,
	    peerDigestSwapInCBlock, fetch);
    } else {
	/* need more data, do we have space? */
	if (size >= SM_PAGE_SIZE)
	    peerDigestFetchAbort(fetch, buf, "stored header too big");
	else
	    storeClientCopy(fetch->sc, fetch->entry, size, 0, SM_PAGE_SIZE, buf,
		peerDigestSwapInHeaders, fetch);
    }
}

static void
peerDigestSwapInCBlock(void *data, char *buf, ssize_t size)
{
    DigestFetchState *fetch = data;

    if (peerDigestFetchedEnough(fetch, buf, size, "peerDigestSwapInCBlock"))
	return;

    if (size >= StoreDigestCBlockSize) {
	PeerDigest *pd = fetch->pd;
	HttpReply *rep = fetch->entry->mem_obj->reply;
	const squid_off_t seen = fetch->offset + size;

	assert(pd && rep);
	if (peerDigestSetCBlock(pd, buf)) {
	    /* XXX: soon we will have variable header size */
	    fetch->offset += StoreDigestCBlockSize;
	    /* switch to CD buffer and fetch digest guts */
	    memFree(buf, MEM_4K_BUF);
	    buf = NULL;
	    assert(pd->cd->mask);
	    storeClientCopy(fetch->sc, fetch->entry,
		seen,
		fetch->offset,
		pd->cd->mask_size,
		pd->cd->mask,
		peerDigestSwapInMask, fetch);
	} else {
	    peerDigestFetchAbort(fetch, buf, "invalid digest cblock");
	}
    } else {
	/* need more data, do we have space? */
	if (size >= SM_PAGE_SIZE)
	    peerDigestFetchAbort(fetch, buf, "digest cblock too big");
	else
	    storeClientCopy(fetch->sc, fetch->entry, size, 0, SM_PAGE_SIZE, buf,
		peerDigestSwapInCBlock, fetch);
    }
}

static void
peerDigestSwapInMask(void *data, char *buf, ssize_t size)
{
    DigestFetchState *fetch = data;
    PeerDigest *pd;

    /* NOTE! buf points to the middle of pd->cd->mask! */
    if (peerDigestFetchedEnough(fetch, NULL, size, "peerDigestSwapInMask"))
	return;

    pd = fetch->pd;
    assert(pd->cd && pd->cd->mask);

    fetch->offset += size;
    fetch->mask_offset += size;
    if (fetch->mask_offset >= pd->cd->mask_size) {
	debug(72, 2) ("peerDigestSwapInMask: Done! Got %" PRINTF_OFF_T ", expected %d\n",
	    fetch->mask_offset, pd->cd->mask_size);
	assert(fetch->mask_offset == pd->cd->mask_size);
	assert(peerDigestFetchedEnough(fetch, NULL, 0, "peerDigestSwapInMask"));
    } else {
	const size_t buf_sz = pd->cd->mask_size - fetch->mask_offset;
	assert(buf_sz > 0);
	storeClientCopy(fetch->sc, fetch->entry,
	    fetch->offset,
	    fetch->offset,
	    buf_sz,
	    pd->cd->mask + fetch->mask_offset,
	    peerDigestSwapInMask, fetch);
    }
}

static int
peerDigestFetchedEnough(DigestFetchState * fetch, char *buf, ssize_t size, const char *step_name)
{
    PeerDigest *pd = NULL;
    const char *host = "<unknown>";	/* peer host */
    const char *reason = NULL;	/* reason for completion */
    const char *no_bug = NULL;	/* successful completion if set */
    const int fcb_valid = cbdataValid(fetch);
    const int pdcb_valid = fcb_valid && cbdataValid(fetch->pd);
    const int pcb_valid = pdcb_valid && cbdataValid(fetch->pd->peer);

    /* test possible exiting conditions (the same for most steps!)
     * cases marked with '?!' should not happen */

    if (!reason) {
	if (!fcb_valid)
	    reason = "fetch aborted?!";
	else if (!(pd = fetch->pd))
	    reason = "peer digest disappeared?!";
#if DONT
	else if (!cbdataValid(pd))
	    reason = "invalidated peer digest?!";
#endif
	else
	    host = strBuf(pd->host);
    }
    debug(72, 6) ("%s: peer %s, offset: %" PRINTF_OFF_T " size: %d.\n",
	step_name, host, fcb_valid ? fetch->offset : (squid_off_t) - 1, (int) size);

    /* continue checking (with pd and host known and valid) */
    if (!reason) {
	if (!cbdataValid(pd->peer))
	    reason = "peer disappeared";
	else if (size < 0)
	    reason = "swap failure";
	else if (!fetch->entry)
	    reason = "swap aborted?!";
	else if (EBIT_TEST(fetch->entry->flags, ENTRY_ABORTED))
	    reason = "swap aborted";
    }
    /* continue checking (maybe-successful eof case) */
    if (!reason && !size) {
	if (!pd->cd)
	    reason = "null digest?!";
	else if (fetch->mask_offset != pd->cd->mask_size)
	    reason = "premature end of digest?!";
	else if (!peerDigestUseful(pd))
	    reason = "useless digest";
	else
	    reason = no_bug = "success";
    }
    /* finish if we have a reason */
    if (reason) {
	const int level = strstr(reason, "?!") ? 1 : 3;
	debug(72, level) ("%s: peer %s, exiting after '%s'\n",
	    step_name, host, reason);
	peerDigestReqFinish(fetch, buf,
	    fcb_valid, pdcb_valid, pcb_valid, reason, !no_bug);
    } else {
	/* paranoid check */
	assert(fcb_valid && pdcb_valid && pcb_valid);
    }
    return reason != NULL;
}

/* call this when all callback data is valid and fetch must be stopped but
 * no error has occurred (e.g. we received 304 reply and reuse old digest) */
static void
peerDigestFetchStop(DigestFetchState * fetch, char *buf, const char *reason)
{
    assert(reason);
    debug(72, 2) ("peerDigestFetchStop: peer %s, reason: %s\n",
	strBuf(fetch->pd->host), reason);
    peerDigestReqFinish(fetch, buf, 1, 1, 1, reason, 0);
}

/* call this when all callback data is valid but something bad happened */
static void
peerDigestFetchAbort(DigestFetchState * fetch, char *buf, const char *reason)
{
    assert(reason);
    debug(72, 2) ("peerDigestFetchAbort: peer %s, reason: %s\n",
	strBuf(fetch->pd->host), reason);
    peerDigestReqFinish(fetch, buf, 1, 1, 1, reason, 1);
}

/* complete the digest transfer, update stats, unlock/release everything */
static void
peerDigestReqFinish(DigestFetchState * fetch, char *buf,
    int fcb_valid, int pdcb_valid, int pcb_valid,
    const char *reason, int err)
{
    assert(reason);

    /* must go before peerDigestPDFinish */
    if (pdcb_valid) {
	fetch->pd->flags.requested = 0;
	fetch->pd->req_result = reason;
    }
    /* schedule next check if peer is still out there */
    if (pcb_valid) {
	PeerDigest *pd = fetch->pd;
	if (err) {
	    pd->times.retry_delay = peerDigestIncDelay(pd);
	    peerDigestSetCheck(pd, pd->times.retry_delay);
	} else {
	    pd->times.retry_delay = 0;
	    peerDigestSetCheck(pd, peerDigestNewDelay(fetch->entry));
	}
    }
    /* note: order is significant */
    if (fcb_valid)
	peerDigestFetchSetStats(fetch);
    if (pdcb_valid)
	peerDigestPDFinish(fetch, pcb_valid, err);
    if (fcb_valid)
	peerDigestFetchFinish(fetch, err);
    if (buf)
	memFree(buf, MEM_4K_BUF);
}


/* destroys digest if peer disappeared
 * must be called only when fetch and pd cbdata are valid */
static void
peerDigestPDFinish(DigestFetchState * fetch, int pcb_valid, int err)
{
    PeerDigest *pd = fetch->pd;
    const char *host = strBuf(pd->host);

    pd->times.received = squid_curtime;
    pd->times.req_delay = fetch->resp_time;
    kb_incr(&pd->stats.sent.kbytes, (size_t) fetch->sent.bytes);
    kb_incr(&pd->stats.recv.kbytes, (size_t) fetch->recv.bytes);
    pd->stats.sent.msgs += fetch->sent.msg;
    pd->stats.recv.msgs += fetch->recv.msg;

    if (err) {
	debug(72, 1) ("%sdisabling (%s) digest from %s\n",
	    pcb_valid ? "temporary " : "",
	    pd->req_result, host);

	if (pd->cd) {
	    cacheDigestDestroy(pd->cd);
	    pd->cd = NULL;
	}
	pd->flags.usable = 0;

	if (!pcb_valid)
	    peerDigestNotePeerGone(pd);
    } else {
	assert(pcb_valid);

	pd->flags.usable = 1;

	/* XXX: ugly condition, but how? */
	if (fetch->entry->store_status == STORE_OK)
	    debug(72, 2) ("re-used old digest from %s\n", host);
	else
	    debug(72, 2) ("received valid digest from %s\n", host);
    }
    fetch->pd = NULL;
    cbdataUnlock(pd);
}

/* free fetch state structures
 * must be called only when fetch cbdata is valid */
static void
peerDigestFetchFinish(DigestFetchState * fetch, int err)
{
    assert(fetch->entry && fetch->request);

    if (fetch->old_entry) {
	debug(72, 2) ("peerDigestFetchFinish: deleting old entry\n");
	storeUnregister(fetch->old_sc, fetch->old_entry, fetch);
	storeReleaseRequest(fetch->old_entry);
	storeUnlockObject(fetch->old_entry);
	fetch->old_entry = NULL;
    }
    /* update global stats */
    kb_incr(&statCounter.cd.kbytes_sent, (size_t) fetch->sent.bytes);
    kb_incr(&statCounter.cd.kbytes_recv, (size_t) fetch->recv.bytes);
    statCounter.cd.msgs_sent += fetch->sent.msg;
    statCounter.cd.msgs_recv += fetch->recv.msg;

    /* unlock everything */
    storeUnregister(fetch->sc, fetch->entry, fetch);
    storeUnlockObject(fetch->entry);
    requestUnlink(fetch->request);
    fetch->entry = NULL;
    fetch->request = NULL;
    assert(fetch->pd == NULL);
    cbdataUnlock(fetch);
    cbdataFree(fetch);
}

/* calculate fetch stats after completion */
static void
peerDigestFetchSetStats(DigestFetchState * fetch)
{
    MemObject *mem;
    assert(fetch->entry && fetch->request);

    mem = fetch->entry->mem_obj;
    assert(mem);

    /* XXX: outgoing numbers are not precise */
    /* XXX: we must distinguish between 304 hits and misses here */
    fetch->sent.bytes = httpRequestPrefixLen(fetch->request);
    fetch->recv.bytes = fetch->entry->store_status == STORE_PENDING ?
	mem->inmem_hi : mem->object_sz;
    fetch->sent.msg = fetch->recv.msg = 1;
    fetch->expires = fetch->entry->expires;
    fetch->resp_time = squid_curtime - fetch->start_time;

    debug(72, 3) ("peerDigestFetchFinish: recv %d bytes in %d secs\n",
	fetch->recv.bytes, (int) fetch->resp_time);
    debug(72, 3) ("peerDigestFetchFinish: expires: %ld (%+d), lmt: %ld (%+d)\n",
	(long int) fetch->expires, (int) (fetch->expires - squid_curtime),
	(long int) fetch->entry->lastmod, (int) (fetch->entry->lastmod - squid_curtime));
}


static int
peerDigestSetCBlock(PeerDigest * pd, const char *buf)
{
    StoreDigestCBlock cblock;
    int freed_size = 0;
    const char *host = strBuf(pd->host);

    xmemcpy(&cblock, buf, sizeof(cblock));
    /* network -> host conversions */
    cblock.ver.current = ntohs(cblock.ver.current);
    cblock.ver.required = ntohs(cblock.ver.required);
    cblock.capacity = ntohl(cblock.capacity);
    cblock.count = ntohl(cblock.count);
    cblock.del_count = ntohl(cblock.del_count);
    cblock.mask_size = ntohl(cblock.mask_size);
    debug(72, 2) ("got digest cblock from %s; ver: %d (req: %d)\n",
	host, (int) cblock.ver.current, (int) cblock.ver.required);
    debug(72, 2) ("\t size: %d bytes, e-cnt: %d, e-util: %d%%\n",
	cblock.mask_size, cblock.count,
	xpercentInt(cblock.count, cblock.capacity));
    /* check version requirements (both ways) */
    if (cblock.ver.required > CacheDigestVer.current) {
	debug(72, 1) ("%s digest requires version %d; have: %d\n",
	    host, cblock.ver.required, CacheDigestVer.current);
	return 0;
    }
    if (cblock.ver.current < CacheDigestVer.required) {
	debug(72, 1) ("%s digest is version %d; we require: %d\n",
	    host, cblock.ver.current, CacheDigestVer.required);
	return 0;
    }
    /* check consistency */
    if (cblock.ver.required > cblock.ver.current ||
	cblock.mask_size <= 0 || cblock.capacity <= 0 ||
	cblock.bits_per_entry <= 0 || cblock.hash_func_count <= 0) {
	debug(72, 0) ("%s digest cblock is corrupted.\n", host);
	return 0;
    }
    /* check consistency further */
    if (cblock.mask_size != cacheDigestCalcMaskSize(cblock.capacity, cblock.bits_per_entry)) {
	debug(72, 0) ("%s digest cblock is corrupted (mask size mismatch: %d ? %d).\n",
	    host, cblock.mask_size, (int) cacheDigestCalcMaskSize(cblock.capacity, cblock.bits_per_entry));
	return 0;
    }
    /* there are some things we cannot do yet */
    if (cblock.hash_func_count != CacheDigestHashFuncCount) {
	debug(72, 0) ("%s digest: unsupported #hash functions: %d ? %d.\n",
	    host, cblock.hash_func_count, CacheDigestHashFuncCount);
	return 0;
    }
    /*
     * no cblock bugs below this point
     */
    /* check size changes */
    if (pd->cd && cblock.mask_size != pd->cd->mask_size) {
	debug(72, 2) ("%s digest changed size: %d -> %d\n",
	    host, cblock.mask_size, pd->cd->mask_size);
	freed_size = pd->cd->mask_size;
	cacheDigestDestroy(pd->cd);
	pd->cd = NULL;
    }
    if (!pd->cd) {
	debug(72, 2) ("creating %s digest; size: %d (%+d) bytes\n",
	    host, cblock.mask_size, (int) (cblock.mask_size - freed_size));
	pd->cd = cacheDigestCreate(cblock.capacity, cblock.bits_per_entry);
	if (cblock.mask_size >= freed_size)
	    kb_incr(&statCounter.cd.memory, cblock.mask_size - freed_size);
    }
    assert(pd->cd);
    /* these assignments leave us in an inconsistent state until we finish reading the digest */
    pd->cd->count = cblock.count;
    pd->cd->del_count = cblock.del_count;
    return 1;
}

static int
peerDigestUseful(const PeerDigest * pd)
{
    /* TODO: we should calculate the prob of a false hit instead of bit util */
    const int bit_util = cacheDigestBitUtil(pd->cd);
    if (bit_util > 65) {
	debug(72, 0) ("Warning: %s peer digest has too many bits on (%d%%).\n",
	    strBuf(pd->host), bit_util);
	return 0;
    }
    return 1;
}

static int
saneDiff(time_t diff)
{
    return abs(diff) > squid_curtime / 2 ? 0 : diff;
}

void
peerDigestStatsReport(const PeerDigest * pd, StoreEntry * e)
{
#define f2s(flag) (pd->flags.flag ? "yes" : "no")
#define appendTime(tm) storeAppendPrintf(e, "%s\t %10ld\t %+d\t %+d\n", \
    ""#tm, (long int)pd->times.tm, \
    saneDiff(pd->times.tm - squid_curtime), \
    saneDiff(pd->times.tm - pd->times.initialized))

    const char *host = pd ? strBuf(pd->host) : NULL;
    assert(pd);

    storeAppendPrintf(e, "\npeer digest from %s\n", host);

    cacheDigestGuessStatsReport(&pd->stats.guess, e, host);

    storeAppendPrintf(e, "\nevent\t timestamp\t secs from now\t secs from init\n");
    appendTime(initialized);
    appendTime(needed);
    appendTime(requested);
    appendTime(received);
    appendTime(next_check);

    storeAppendPrintf(e, "peer digest state:\n");
    storeAppendPrintf(e, "\tneeded: %3s, usable: %3s, requested: %3s\n",
	f2s(needed), f2s(usable), f2s(requested));
    storeAppendPrintf(e, "\n\tlast retry delay: %d secs\n",
	(int) pd->times.retry_delay);
    storeAppendPrintf(e, "\tlast request response time: %d secs\n",
	(int) pd->times.req_delay);
    storeAppendPrintf(e, "\tlast request result: %s\n",
	pd->req_result ? pd->req_result : "(none)");

    storeAppendPrintf(e, "\npeer digest traffic:\n");
    storeAppendPrintf(e, "\trequests sent: %d, volume: %d KB\n",
	pd->stats.sent.msgs, (int) pd->stats.sent.kbytes.kb);
    storeAppendPrintf(e, "\treplies recv:  %d, volume: %d KB\n",
	pd->stats.recv.msgs, (int) pd->stats.recv.kbytes.kb);

    storeAppendPrintf(e, "\npeer digest structure:\n");
    if (pd->cd)
	cacheDigestReport(pd->cd, host, e);
    else
	storeAppendPrintf(e, "\tno in-memory copy\n");
}

#endif
