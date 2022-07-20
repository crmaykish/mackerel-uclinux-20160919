
/*
 * $Id: HttpReply.c,v 1.49.2.5 2005/03/26 02:50:51 hno Exp $
 *
 * DEBUG: section 58    HTTP Reply (Response)
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


/* local constants */

/* these entity-headers must be ignored if a bogus server sends them in 304 */
static HttpHeaderMask Denied304HeadersMask;
static http_hdr_type Denied304HeadersArr[] =
{
    HDR_ALLOW, HDR_CONTENT_ENCODING, HDR_CONTENT_LANGUAGE, HDR_CONTENT_LENGTH,
    HDR_CONTENT_LOCATION, HDR_CONTENT_RANGE, HDR_LAST_MODIFIED, HDR_LINK,
    HDR_OTHER
};

/* local routines */
static void httpReplyInit(HttpReply * rep);
static void httpReplyClean(HttpReply * rep);
static void httpReplyDoDestroy(HttpReply * rep);
static void httpReplyHdrCacheInit(HttpReply * rep);
static void httpReplyHdrCacheClean(HttpReply * rep);
static int httpReplyParseStep(HttpReply * rep, const char *parse_start, int atEnd);
static int httpReplyParseError(HttpReply * rep);
static int httpReplyIsolateStart(const char **parse_start, const char **blk_start, const char **blk_end);
static time_t httpReplyHdrExpirationTime(const HttpReply * rep);


/* module initialization */
void
httpReplyInitModule(void)
{
    httpHeaderMaskInit(&Denied304HeadersMask, 0);
    httpHeaderCalcMask(&Denied304HeadersMask, (const int *) Denied304HeadersArr, countof(Denied304HeadersArr));
}


HttpReply *
httpReplyCreate(void)
{
    HttpReply *rep = memAllocate(MEM_HTTP_REPLY);
    debug(58, 7) ("creating rep: %p\n", rep);
    httpReplyInit(rep);
    return rep;
}

static void
httpReplyInit(HttpReply * rep)
{
    assert(rep);
    rep->hdr_sz = 0;
    rep->pstate = psReadyToParseStartLine;
    httpBodyInit(&rep->body);
    httpHeaderInit(&rep->header, hoReply);
    httpReplyHdrCacheInit(rep);
    httpStatusLineInit(&rep->sline);
}

static void
httpReplyClean(HttpReply * rep)
{
    assert(rep);
    httpBodyClean(&rep->body);
    httpReplyHdrCacheClean(rep);
    httpHeaderClean(&rep->header);
    httpStatusLineClean(&rep->sline);
}

void
httpReplyDestroy(HttpReply * rep)
{
    assert(rep);
    debug(58, 7) ("destroying rep: %p\n", rep);
    httpReplyClean(rep);
    httpReplyDoDestroy(rep);
}

void
httpReplyReset(HttpReply * rep)
{
    httpReplyClean(rep);
    httpReplyInit(rep);
}

/* absorb: copy the contents of a new reply to the old one, destroy new one */
void
httpReplyAbsorb(HttpReply * rep, HttpReply * new_rep)
{
    assert(rep && new_rep);
    httpReplyClean(rep);
    *rep = *new_rep;
    /* cannot use Clean() on new reply now! */
    httpReplyDoDestroy(new_rep);
}

/*
 * httpReplyParse takes character buffer of HTTP headers (buf),
 * which may not be NULL-terminated, and fills in an HttpReply
 * structure (rep).  The parameter 'end' specifies the offset to
 * the end of the reply headers.  The caller may know where the
 * end is, but is unable to NULL-terminate the buffer.  This function
 * returns true on success.
 */
int
httpReplyParse(HttpReply * rep, const char *buf, size_t end)
{
    /*
     * this extra buffer/copy will be eliminated when headers become
     * meta-data in store. Currently we have to xstrncpy the buffer
     * becuase somebody may feed a non NULL-terminated buffer to
     * us.
     */
    MemBuf mb = MemBufNull;
    int success;
    /* reset current state, because we are not used in incremental fashion */
    httpReplyReset(rep);
    /* put a string terminator.  s is how many bytes to touch in
     * 'buf' including the terminating NULL. */
    memBufDefInit(&mb);
    memBufAppend(&mb, buf, end);
    memBufAppend(&mb, "\0", 1);
    success = httpReplyParseStep(rep, mb.buf, 0);
    memBufClean(&mb);
    return success == 1;
}

void
httpReplyPackInto(const HttpReply * rep, Packer * p)
{
    assert(rep);
    httpStatusLinePackInto(&rep->sline, p);
    httpHeaderPackInto(&rep->header, p);
    packerAppend(p, "\r\n", 2);
    httpBodyPackInto(&rep->body, p);
}

/* create memBuf, create mem-based packer,  pack, destroy packer, return MemBuf */
MemBuf
httpReplyPack(const HttpReply * rep)
{
    MemBuf mb;
    Packer p;
    assert(rep);

    memBufDefInit(&mb);
    packerToMemInit(&p, &mb);
    httpReplyPackInto(rep, &p);
    packerClean(&p);
    return mb;
}

/* swap: create swap-based packer, pack, destroy packer */
void
httpReplySwapOut(const HttpReply * rep, StoreEntry * e)
{
    Packer p;
    assert(rep && e);

    packerToStoreInit(&p, e);
    httpReplyPackInto(rep, &p);
    packerClean(&p);
}

#if UNUSED_CODE
MemBuf
httpPackedReply(http_version_t ver, http_status status, const char *ctype,
    squid_off_t clen, time_t lmt, time_t expires)
{
    HttpReply *rep = httpReplyCreate();
    MemBuf mb;
    httpReplySetHeaders(rep, ver, status, ctype, NULL, clen, lmt, expires);
    mb = httpReplyPack(rep);
    httpReplyDestroy(rep);
    return mb;
}
#endif

MemBuf
httpPacked304Reply(const HttpReply * rep)
{
    static const http_hdr_type ImsEntries[] =
    {HDR_DATE, HDR_CONTENT_TYPE, HDR_EXPIRES, HDR_LAST_MODIFIED, /* eof */ HDR_OTHER};
    int t;
    MemBuf mb;
    Packer p;
    HttpHeaderEntry *e;
    assert(rep);

    memBufDefInit(&mb);
    packerToMemInit(&p, &mb);
    memBufPrintf(&mb, "%s", "HTTP/1.0 304 Not Modified\r\n");
    for (t = 0; ImsEntries[t] != HDR_OTHER; ++t)
	if ((e = httpHeaderFindEntry(&rep->header, ImsEntries[t])))
	    httpHeaderEntryPackInto(e, &p);
    memBufAppend(&mb, "\r\n", 2);
    packerClean(&p);
    return mb;
}

void
httpReplySetHeaders(HttpReply * reply, http_version_t ver, http_status status, const char *reason,
    const char *ctype, squid_off_t clen, time_t lmt, time_t expires)
{
    HttpHeader *hdr;
    assert(reply);
    httpStatusLineSet(&reply->sline, ver, status, reason);
    hdr = &reply->header;
    httpHeaderPutStr(hdr, HDR_SERVER, full_appname_string);
    httpHeaderPutStr(hdr, HDR_MIME_VERSION, "1.0");
    httpHeaderPutTime(hdr, HDR_DATE, squid_curtime);
    if (ctype) {
	httpHeaderPutStr(hdr, HDR_CONTENT_TYPE, ctype);
	stringInit(&reply->content_type, ctype);
    } else
	reply->content_type = StringNull;
    if (clen >= 0)
	httpHeaderPutSize(hdr, HDR_CONTENT_LENGTH, clen);
    if (expires >= 0)
	httpHeaderPutTime(hdr, HDR_EXPIRES, expires);
    if (lmt > 0)		/* this used to be lmt != 0 @?@ */
	httpHeaderPutTime(hdr, HDR_LAST_MODIFIED, lmt);
    reply->date = squid_curtime;
    reply->content_length = clen;
    reply->expires = expires;
    reply->last_modified = lmt;
}

void
httpRedirectReply(HttpReply * reply, http_status status, const char *loc)
{
    HttpHeader *hdr;
    http_version_t ver;
    assert(reply);
    httpBuildVersion(&ver, 1, 0);
    httpStatusLineSet(&reply->sline, ver, status, httpStatusString(status));
    hdr = &reply->header;
    httpHeaderPutStr(hdr, HDR_SERVER, full_appname_string);
    httpHeaderPutTime(hdr, HDR_DATE, squid_curtime);
    httpHeaderPutSize(hdr, HDR_CONTENT_LENGTH, 0);
    httpHeaderPutStr(hdr, HDR_LOCATION, loc);
    reply->date = squid_curtime;
    reply->content_length = 0;
}

void
httpReplyUpdateOnNotModified(HttpReply * rep, HttpReply * freshRep)
{
    assert(rep && freshRep);
    /* clean cache */
    httpReplyHdrCacheClean(rep);
    /* update raw headers */
    httpHeaderUpdate(&rep->header, &freshRep->header,
	(const HttpHeaderMask *) &Denied304HeadersMask);
    /* init cache */
    httpReplyHdrCacheInit(rep);
}


/* internal routines */

/* internal function used by Destroy and Absorb */
static void
httpReplyDoDestroy(HttpReply * rep)
{
    memFree(rep, MEM_HTTP_REPLY);
}

static time_t
httpReplyHdrExpirationTime(const HttpReply * rep)
{
    /* The s-maxage and max-age directive takes priority over Expires */
    if (rep->cache_control) {
	if (rep->date >= 0) {
	    if (rep->cache_control->s_maxage >= 0)
		return rep->date + rep->cache_control->s_maxage;
	    if (rep->cache_control->max_age >= 0)
		return rep->date + rep->cache_control->max_age;
	} else {
	    /*
	     * Conservatively handle the case when we have a max-age
	     * header, but no Date for reference?
	     */
	    if (rep->cache_control->s_maxage >= 0)
		return squid_curtime;
	    if (rep->cache_control->max_age >= 0)
		return squid_curtime;
	}
    }
    if (Config.onoff.vary_ignore_expire &&
	httpHeaderHas(&rep->header, HDR_VARY)) {
	const time_t d = httpHeaderGetTime(&rep->header, HDR_DATE);
	const time_t e = httpHeaderGetTime(&rep->header, HDR_EXPIRES);
	if (d == e)
	    return -1;
    }
    if (httpHeaderHas(&rep->header, HDR_EXPIRES)) {
	const time_t e = httpHeaderGetTime(&rep->header, HDR_EXPIRES);
	/*
	 * HTTP/1.0 says that robust implementations should consider
	 * bad or malformed Expires header as equivalent to "expires
	 * immediately."
	 */
	return e < 0 ? squid_curtime : e;
    }
    return -1;
}

/* sync this routine when you update HttpReply struct */
static void
httpReplyHdrCacheInit(HttpReply * rep)
{
    const HttpHeader *hdr = &rep->header;
    const char *str;
    rep->content_length = httpHeaderGetSize(hdr, HDR_CONTENT_LENGTH);
    rep->date = httpHeaderGetTime(hdr, HDR_DATE);
    rep->last_modified = httpHeaderGetTime(hdr, HDR_LAST_MODIFIED);
    str = httpHeaderGetStr(hdr, HDR_CONTENT_TYPE);
    if (str)
	stringLimitInit(&rep->content_type, str, strcspn(str, ";\t "));
    else
	rep->content_type = StringNull;
    rep->cache_control = httpHeaderGetCc(hdr);
    rep->content_range = httpHeaderGetContRange(hdr);
    rep->keep_alive = httpMsgIsPersistent(rep->sline.version, &rep->header);
    /* be sure to set expires after date and cache-control */
    rep->expires = httpReplyHdrExpirationTime(rep);
}

/* sync this routine when you update HttpReply struct */
static void
httpReplyHdrCacheClean(HttpReply * rep)
{
    stringClean(&rep->content_type);
    if (rep->cache_control)
	httpHdrCcDestroy(rep->cache_control);
    if (rep->content_range)
	httpHdrContRangeDestroy(rep->content_range);
}

/*
 * parses a 0-terminating buffer into HttpReply. 
 * Returns:
 *      +1 -- success 
 *       0 -- need more data (partial parse)
 *      -1 -- parse error
 */
static int
httpReplyParseStep(HttpReply * rep, const char *buf, int atEnd)
{
    const char *parse_start = buf;
    const char *blk_start, *blk_end;
    const char **parse_end_ptr = &blk_end;
    assert(rep);
    assert(parse_start);
    assert(rep->pstate < psParsed);

    *parse_end_ptr = parse_start;
    if (rep->pstate == psReadyToParseStartLine) {
	if (!httpReplyIsolateStart(&parse_start, &blk_start, &blk_end))
	    return 0;
	if (!httpStatusLineParse(&rep->sline, blk_start, blk_end))
	    return httpReplyParseError(rep);

	*parse_end_ptr = parse_start;
	rep->hdr_sz = *parse_end_ptr - buf;
	rep->pstate++;
    }
    if (rep->pstate == psReadyToParseHeaders) {
	if (!httpMsgIsolateHeaders(&parse_start, &blk_start, &blk_end)) {
	    if (atEnd)
		blk_start = parse_start, blk_end = blk_start + strlen(blk_start);
	    else
		return 0;
	}
	if (!httpHeaderParse(&rep->header, blk_start, blk_end))
	    return httpReplyParseError(rep);

	httpReplyHdrCacheInit(rep);

	*parse_end_ptr = parse_start;
	rep->hdr_sz = *parse_end_ptr - buf;
	rep->pstate++;
    }
    return 1;
}

/* handy: resets and returns -1 */
static int
httpReplyParseError(HttpReply * rep)
{
    assert(rep);
    /* reset */
    httpReplyReset(rep);
    /* indicate an error */
    rep->sline.status = HTTP_INVALID_HEADER;
    return -1;
}

/* find first CRLF */
static int
httpReplyIsolateStart(const char **parse_start, const char **blk_start, const char **blk_end)
{
    int slen = strcspn(*parse_start, "\r\n");
    if (!(*parse_start)[slen])	/* no CRLF found */
	return 0;

    *blk_start = *parse_start;
    *blk_end = *blk_start + slen;
    while (**blk_end == '\r')	/* CR */
	(*blk_end)++;
    if (**blk_end == '\n')	/* LF */
	(*blk_end)++;

    *parse_start = *blk_end;
    return 1;
}

/*
 * Returns the body size of a HTTP response
 */
squid_off_t
httpReplyBodySize(method_t method, const HttpReply * reply)
{
    if (reply->sline.version.major < 1)
	return -1;
    else if (METHOD_HEAD == method)
	return 0;
    else if (reply->sline.status == HTTP_OK)
	(void) 0;		/* common case, continue */
    else if (reply->sline.status == HTTP_NO_CONTENT)
	return 0;
    else if (reply->sline.status == HTTP_NOT_MODIFIED)
	return 0;
    else if (reply->sline.status < HTTP_OK)
	return 0;
    return reply->content_length;
}
