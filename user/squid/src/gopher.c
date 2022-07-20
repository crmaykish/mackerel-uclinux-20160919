
/*
 * $Id: gopher.c,v 1.162.2.8 2005/01/12 06:04:01 wessels Exp $
 *
 * DEBUG: section 10    Gopher
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

/* gopher type code from rfc. Anawat. */
#define GOPHER_FILE         '0'
#define GOPHER_DIRECTORY    '1'
#define GOPHER_CSO          '2'
#define GOPHER_ERROR        '3'
#define GOPHER_MACBINHEX    '4'
#define GOPHER_DOSBIN       '5'
#define GOPHER_UUENCODED    '6'
#define GOPHER_INDEX        '7'
#define GOPHER_TELNET       '8'
#define GOPHER_BIN          '9'
#define GOPHER_REDUNT       '+'
#define GOPHER_3270         'T'
#define GOPHER_GIF          'g'
#define GOPHER_IMAGE        'I'

#define GOPHER_HTML         'h'	/* HTML */
#define GOPHER_INFO         'i'
#define GOPHER_WWW          'w'	/* W3 address */
#define GOPHER_SOUND        's'

#define GOPHER_PLUS_IMAGE   ':'
#define GOPHER_PLUS_MOVIE   ';'
#define GOPHER_PLUS_SOUND   '<'

#define GOPHER_PORT         70

#define TAB                 '\t'
#define TEMP_BUF_SIZE       4096
#define MAX_CSO_RESULT      1024

typedef struct gopher_ds {
    StoreEntry *entry;
    enum {
	NORMAL,
	HTML_DIR,
	HTML_INDEX_RESULT,
	HTML_CSO_RESULT,
	HTML_INDEX_PAGE,
	HTML_CSO_PAGE
    } conversion;
    int HTML_header_added;
    int HTML_pre;
    char type_id;
    char request[MAX_URL];
    int cso_recno;
    int len;
    char *buf;			/* pts to a 4k page */
    int fd;
    request_t *req;
    FwdState *fwdState;
} GopherStateData;

static PF gopherStateFree;
static void gopher_mime_content(MemBuf * mb, const char *name, const char *def);
static void gopherMimeCreate(GopherStateData *);
static void gopher_request_parse(const request_t * req,
    char *type_id,
    char *request);
static void gopherEndHTML(GopherStateData *);
static void gopherToHTML(GopherStateData *, char *inbuf, int len);
static PF gopherTimeout;
static PF gopherReadReply;
static CWCB gopherSendComplete;
static PF gopherSendRequest;

static char def_gopher_bin[] = "www/unknown";
static char def_gopher_text[] = "text/plain";

static void
gopherStateFree(int fdnotused, void *data)
{
    GopherStateData *gopherState = data;
    if (gopherState == NULL)
	return;
    if (gopherState->entry) {
	storeUnlockObject(gopherState->entry);
    }
    if (gopherState->req) {
	requestUnlink(gopherState->req);
    }
    memFree(gopherState->buf, MEM_4K_BUF);
    gopherState->buf = NULL;
    cbdataFree(gopherState);
}


/* figure out content type from file extension */
static void
gopher_mime_content(MemBuf * mb, const char *name, const char *def_ctype)
{
    char *ctype = mimeGetContentType(name);
    char *cenc = mimeGetContentEncoding(name);
    if (cenc)
	memBufPrintf(mb, "Content-Encoding: %s\r\n", cenc);
    memBufPrintf(mb, "Content-Type: %s\r\n",
	ctype ? ctype : def_ctype);
}



/* create MIME Header for Gopher Data */
static void
gopherMimeCreate(GopherStateData * gopherState)
{
    MemBuf mb;

    memBufDefInit(&mb);

    memBufPrintf(&mb,
	"HTTP/1.0 200 OK Gatewaying\r\n"
	"Server: Squid/%s\r\n"
	"Date: %s\r\n",
	version_string, mkrfc1123(squid_curtime));

    switch (gopherState->type_id) {

    case GOPHER_DIRECTORY:
    case GOPHER_INDEX:
    case GOPHER_HTML:
    case GOPHER_WWW:
    case GOPHER_CSO:
	memBufPrintf(&mb, "Content-Type: text/html\r\n");
	break;
    case GOPHER_GIF:
    case GOPHER_IMAGE:
    case GOPHER_PLUS_IMAGE:
	memBufPrintf(&mb, "Content-Type: image/gif\r\n");
	break;
    case GOPHER_SOUND:
    case GOPHER_PLUS_SOUND:
	memBufPrintf(&mb, "Content-Type: audio/basic\r\n");
	break;
    case GOPHER_PLUS_MOVIE:
	memBufPrintf(&mb, "Content-Type: video/mpeg\r\n");
	break;
    case GOPHER_MACBINHEX:
    case GOPHER_DOSBIN:
    case GOPHER_UUENCODED:
    case GOPHER_BIN:
	/* Rightnow We have no idea what it is. */
	gopher_mime_content(&mb, gopherState->request, def_gopher_bin);
	break;
    case GOPHER_FILE:
    default:
	gopher_mime_content(&mb, gopherState->request, def_gopher_text);
	break;
    }
    memBufPrintf(&mb, "\r\n");
    EBIT_CLR(gopherState->entry->flags, ENTRY_FWD_HDR_WAIT);
    storeAppend(gopherState->entry, mb.buf, mb.size);
    memBufClean(&mb);
}

/* Parse a gopher request into components.  By Anawat. */
static void
gopher_request_parse(const request_t * req, char *type_id, char *request)
{
    const char *path = strBuf(req->urlpath);

    if (request)
	request[0] = '\0';

    if (path && (*path == '/'))
	path++;

    if (!path || !*path) {
	*type_id = GOPHER_DIRECTORY;
	return;
    }
    *type_id = path[0];

    if (request) {
	xstrncpy(request, path + 1, MAX_URL);
	/* convert %xx to char */
	url_convert_hex(request, 0);
    }
}

int
gopherCachable(const request_t * req)
{
    int cachable = 1;
    char type_id;
    /* parse to see type */
    gopher_request_parse(req,
	&type_id,
	NULL);
    switch (type_id) {
    case GOPHER_INDEX:
    case GOPHER_CSO:
    case GOPHER_TELNET:
    case GOPHER_3270:
	cachable = 0;
	break;
    default:
	cachable = 1;
    }
    return cachable;
}

static void
gopherHTMLHeader(StoreEntry * e, const char *title, const char *substring)
{
    storeAppendPrintf(e, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
    storeAppendPrintf(e, "<HTML><HEAD><TITLE>");
    storeAppendPrintf(e, title, substring);
    storeAppendPrintf(e, "</TITLE>");
    storeAppendPrintf(e, "<STYLE type=\"text/css\"><!--BODY{background-color:#ffffff;font-family:verdana,sans-serif}--></STYLE>\n");
    storeAppendPrintf(e, "</HEAD>\n<BODY><H1>");
    storeAppendPrintf(e, title, substring);
    storeAppendPrintf(e, "</H1>\n");
}

static void
gopherHTMLFooter(StoreEntry * e)
{
    storeAppendPrintf(e, "<HR noshade size=\"1px\">\n");
    storeAppendPrintf(e, "<ADDRESS>\n");
    storeAppendPrintf(e, "Generated %s by %s (%s)\n",
	mkrfc1123(squid_curtime),
	getMyHostname(),
	full_appname_string);
    storeAppendPrintf(e, "</ADDRESS></BODY></HTML>\n");
}

static void
gopherEndHTML(GopherStateData * gopherState)
{
    StoreEntry *e = gopherState->entry;
    if (!gopherState->HTML_header_added) {
	gopherHTMLHeader(e, "Server Return Nothing", NULL);
	storeAppendPrintf(e, "<P>The Gopher query resulted in a blank response</P>");
    } else if (gopherState->HTML_pre) {
	storeAppendPrintf(e, "</PRE>\n");
    }
    gopherHTMLFooter(e);
}


/* Convert Gopher to HTML */
/* Borrow part of code from libwww2 came with Mosaic distribution */
static void
gopherToHTML(GopherStateData * gopherState, char *inbuf, int len)
{
    char *pos = inbuf;
    char *lpos = NULL;
    char *tline = NULL;
    LOCAL_ARRAY(char, line, TEMP_BUF_SIZE);
    LOCAL_ARRAY(char, tmpbuf, TEMP_BUF_SIZE);
    String outbuf = StringNull;
    char *name = NULL;
    char *selector = NULL;
    char *host = NULL;
    char *port = NULL;
    char *escaped_selector = NULL;
    const char *icon_url = NULL;
    char gtype;
    StoreEntry *entry = NULL;

    memset(tmpbuf, '\0', TEMP_BUF_SIZE);
    memset(line, '\0', TEMP_BUF_SIZE);

    entry = gopherState->entry;

    if (gopherState->conversion == HTML_INDEX_PAGE) {
	char *html_url = html_quote(storeUrl(entry));
	gopherHTMLHeader(entry, "Gopher Index %s", html_url);
	storeAppendPrintf(entry,
	    "<p>This is a searchable Gopher index. Use the search\n"
	    "function of your browser to enter search terms.\n"
	    "<ISINDEX>\n");
	gopherHTMLFooter(entry);
	/* now let start sending stuff to client */
	storeBufferFlush(entry);
	gopherState->HTML_header_added = 1;
	return;
    }
    if (gopherState->conversion == HTML_CSO_PAGE) {
	char *html_url = html_quote(storeUrl(entry));
	gopherHTMLHeader(entry, "CSO Search of %s", html_url);
	storeAppendPrintf(entry,
	    "<P>A CSO database usually contains a phonebook or\n"
	    "directory.  Use the search function of your browser to enter\n"
	    "search terms.</P><ISINDEX>\n");
	gopherHTMLFooter(entry);
	/* now let start sending stuff to client */
	storeBufferFlush(entry);
	gopherState->HTML_header_added = 1;
	return;
    }
    inbuf[len] = '\0';

    if (!gopherState->HTML_header_added) {
	if (gopherState->conversion == HTML_CSO_RESULT)
	    gopherHTMLHeader(entry, "CSO Search Result", NULL);
	else
	    gopherHTMLHeader(entry, "Gopher Menu", NULL);
	strCat(outbuf, "<PRE>");
	gopherState->HTML_header_added = 1;
	gopherState->HTML_pre = 1;
    }
    while ((pos != NULL) && (pos < inbuf + len)) {

	if (gopherState->len != 0) {
	    /* there is something left from last tx. */
	    xstrncpy(line, gopherState->buf, gopherState->len + 1);
	    if (gopherState->len + len > TEMP_BUF_SIZE) {
		debug(10, 1) ("GopherHTML: Buffer overflow. Lost some data on URL: %s\n",
		    storeUrl(entry));
		len = TEMP_BUF_SIZE - gopherState->len;
	    }
	    lpos = (char *) memccpy(line + gopherState->len, inbuf, '\n', len);
	    if (lpos)
		*lpos = '\0';
	    else {
		/* there is no complete line in inbuf */
		/* copy it to temp buffer */
		if (gopherState->len + len > TEMP_BUF_SIZE) {
		    debug(10, 1) ("GopherHTML: Buffer overflow. Lost some data on URL: %s\n",
			storeUrl(entry));
		    len = TEMP_BUF_SIZE - gopherState->len;
		}
		xmemcpy(gopherState->buf + gopherState->len, inbuf, len);
		gopherState->len += len;
		return;
	    }

	    /* skip one line */
	    pos = (char *) memchr(pos, '\n', len);
	    if (pos)
		pos++;

	    /* we're done with the remain from last tx. */
	    gopherState->len = 0;
	    *(gopherState->buf) = '\0';
	} else {

	    lpos = (char *) memccpy(line, pos, '\n', len - (pos - inbuf));
	    if (lpos)
		*lpos = '\0';
	    else {
		/* there is no complete line in inbuf */
		/* copy it to temp buffer */
		if ((len - (pos - inbuf)) > TEMP_BUF_SIZE) {
		    debug(10, 1) ("GopherHTML: Buffer overflow. Lost some data on URL: %s\n",
			storeUrl(entry));
		    len = TEMP_BUF_SIZE;
		}
		if (len > (pos - inbuf)) {
		    xmemcpy(gopherState->buf, pos, len - (pos - inbuf));
		    gopherState->len = len - (pos - inbuf);
		}
		break;
	    }

	    /* skip one line */
	    pos = (char *) memchr(pos, '\n', len);
	    if (pos)
		pos++;

	}

	/* at this point. We should have one line in buffer to process */

	if (*line == '.') {
	    /* skip it */
	    memset(line, '\0', TEMP_BUF_SIZE);
	    continue;
	}
	switch (gopherState->conversion) {

	case HTML_INDEX_RESULT:
	case HTML_DIR:{
		tline = line;
		gtype = *tline++;
		name = tline;
		selector = strchr(tline, TAB);
		if (selector) {
		    *selector++ = '\0';
		    host = strchr(selector, TAB);
		    if (host) {
			*host++ = '\0';
			port = strchr(host, TAB);
			if (port) {
			    char *junk;
			    port[0] = ':';
			    junk = strchr(host, TAB);
			    if (junk)
				*junk++ = 0;	/* Chop port */
			    else {
				junk = strchr(host, '\r');
				if (junk)
				    *junk++ = 0;	/* Chop port */
				else {
				    junk = strchr(host, '\n');
				    if (junk)
					*junk++ = 0;	/* Chop port */
				}
			    }
			    if ((port[1] == '0') && (!port[2]))
				port[0] = 0;	/* 0 means none */
			}
			/* escape a selector here */
			escaped_selector = xstrdup(rfc1738_escape_part(selector));

			switch (gtype) {
			case GOPHER_DIRECTORY:
			    icon_url = mimeGetIconURL("internal-menu");
			    break;
			case GOPHER_HTML:
			case GOPHER_FILE:
			    icon_url = mimeGetIconURL("internal-text");
			    break;
			case GOPHER_INDEX:
			case GOPHER_CSO:
			    icon_url = mimeGetIconURL("internal-index");
			    break;
			case GOPHER_IMAGE:
			case GOPHER_GIF:
			case GOPHER_PLUS_IMAGE:
			    icon_url = mimeGetIconURL("internal-image");
			    break;
			case GOPHER_SOUND:
			case GOPHER_PLUS_SOUND:
			    icon_url = mimeGetIconURL("internal-sound");
			    break;
			case GOPHER_PLUS_MOVIE:
			    icon_url = mimeGetIconURL("internal-movie");
			    break;
			case GOPHER_TELNET:
			case GOPHER_3270:
			    icon_url = mimeGetIconURL("internal-telnet");
			    break;
			case GOPHER_BIN:
			case GOPHER_MACBINHEX:
			case GOPHER_DOSBIN:
			case GOPHER_UUENCODED:
			    icon_url = mimeGetIconURL("internal-binary");
			    break;
			case GOPHER_INFO:
			    icon_url = NULL;
			    break;
			default:
			    icon_url = mimeGetIconURL("internal-unknown");
			    break;
			}

			memset(tmpbuf, '\0', TEMP_BUF_SIZE);
			if ((gtype == GOPHER_TELNET) || (gtype == GOPHER_3270)) {
			    if (strlen(escaped_selector) != 0)
				snprintf(tmpbuf, TEMP_BUF_SIZE, "<IMG border=\"0\" SRC=\"%s\"> <A HREF=\"telnet://%s@%s%s%s/\">%s</A>\n",
				    icon_url, escaped_selector, rfc1738_escape_part(host),
				    *port ? ":" : "", port, html_quote(name));
			    else
				snprintf(tmpbuf, TEMP_BUF_SIZE, "<IMG border=\"0\" SRC=\"%s\"> <A HREF=\"telnet://%s%s%s/\">%s</A>\n",
				    icon_url, rfc1738_escape_part(host), *port ? ":" : "",
				    port, html_quote(name));

			} else if (gtype == GOPHER_INFO) {
			    snprintf(tmpbuf, TEMP_BUF_SIZE, "\t%s\n", html_quote(name));
			} else {
			    if (strncmp(selector, "GET /", 5) == 0) {
				/* WWW link */
				snprintf(tmpbuf, TEMP_BUF_SIZE, "<IMG border=\"0\" SRC=\"%s\"> <A HREF=\"http://%s/%s\">%s</A>\n",
				    icon_url, host, rfc1738_escape_unescaped(selector + 5), html_quote(name));
			    } else {
				/* Standard link */
				snprintf(tmpbuf, TEMP_BUF_SIZE, "<IMG border=\"0\" SRC=\"%s\"> <A HREF=\"gopher://%s/%c%s\">%s</A>\n",
				    icon_url, host, gtype, escaped_selector, html_quote(name));
			    }
			}
			safe_free(escaped_selector);
			strCat(outbuf, tmpbuf);
		    } else {
			memset(line, '\0', TEMP_BUF_SIZE);
			continue;
		    }
		} else {
		    memset(line, '\0', TEMP_BUF_SIZE);
		    continue;
		}
		break;
	    }			/* HTML_DIR, HTML_INDEX_RESULT */


	case HTML_CSO_RESULT:{
		if (line[0] == '-') {
		    int code, recno;
		    char *s_code, *s_recno, *result;

		    s_code = strtok(line + 1, ":\n");
		    s_recno = strtok(NULL, ":\n");
		    result = strtok(NULL, "\n");

		    if (!result)
			break;

		    code = atoi(s_code);
		    recno = atoi(s_recno);

		    if (code != 200)
			break;

		    if (gopherState->cso_recno != recno) {
			snprintf(tmpbuf, TEMP_BUF_SIZE, "</PRE><HR noshade size=\"1px\"><H2>Record# %d<br><i>%s</i></H2>\n<PRE>", recno, html_quote(result));
			gopherState->cso_recno = recno;
		    } else {
			snprintf(tmpbuf, TEMP_BUF_SIZE, "%s\n", html_quote(result));
		    }
		    strCat(outbuf, tmpbuf);
		    break;
		} else {
		    int code;
		    char *s_code, *result;

		    s_code = strtok(line, ":");
		    result = strtok(NULL, "\n");

		    if (!result)
			break;

		    code = atoi(s_code);
		    switch (code) {

		    case 200:{
			    /* OK */
			    /* Do nothing here */
			    break;
			}

		    case 102:	/* Number of matches */
		    case 501:	/* No Match */
		    case 502:	/* Too Many Matches */
			{
			    /* Print the message the server returns */
			    snprintf(tmpbuf, TEMP_BUF_SIZE, "</PRE><HR noshade size=\"1px\"><H2>%s</H2>\n<PRE>", html_quote(result));
			    strCat(outbuf, tmpbuf);
			    break;
			}


		    }
		}

	    }			/* HTML_CSO_RESULT */
	default:
	    break;		/* do nothing */

	}			/* switch */

    }				/* while loop */

    if (strLen(outbuf) > 0) {
	storeAppend(entry, strBuf(outbuf), strLen(outbuf));
	/* now let start sending stuff to client */
	storeBufferFlush(entry);
    }
    stringClean(&outbuf);
    return;
}

static void
gopherTimeout(int fd, void *data)
{
    GopherStateData *gopherState = data;
    StoreEntry *entry = gopherState->entry;
    debug(10, 4) ("gopherTimeout: FD %d: '%s'\n", fd, storeUrl(entry));
    if (entry->store_status == STORE_PENDING) {
	if (entry->mem_obj->inmem_hi == 0) {
	    fwdFail(gopherState->fwdState,
		errorCon(ERR_READ_TIMEOUT, HTTP_GATEWAY_TIMEOUT));
	}
    }
    comm_close(fd);
}

/* This will be called when data is ready to be read from fd.  Read until
 * error or connection closed. */
static void
gopherReadReply(int fd, void *data)
{
    GopherStateData *gopherState = data;
    StoreEntry *entry = gopherState->entry;
    char *buf = NULL;
    int len;
    int clen;
    int bin;
    size_t read_sz;
#if DELAY_POOLS
    delay_id delay_id = delayMostBytesAllowed(entry->mem_obj);
#endif
    if (EBIT_TEST(entry->flags, ENTRY_ABORTED)) {
	comm_close(fd);
	return;
    }
    errno = 0;
    buf = memAllocate(MEM_4K_BUF);
    read_sz = 4096 - 1;		/* leave room for termination */
#if DELAY_POOLS
    read_sz = delayBytesWanted(delay_id, 1, read_sz);
#endif
    /* leave one space for \0 in gopherToHTML */
    statCounter.syscalls.sock.reads++;
    len = FD_READ_METHOD(fd, buf, read_sz);
    if (len > 0) {
	fd_bytes(fd, len, FD_READ);
#if DELAY_POOLS
	delayBytesIn(delay_id, len);
#endif
	kb_incr(&statCounter.server.all.kbytes_in, len);
	kb_incr(&statCounter.server.other.kbytes_in, len);
    }
    debug(10, 5) ("gopherReadReply: FD %d read len=%d\n", fd, len);
    if (len > 0) {
	commSetTimeout(fd, Config.Timeout.read, NULL, NULL);
	IOStats.Gopher.reads++;
	for (clen = len - 1, bin = 0; clen; bin++)
	    clen >>= 1;
	IOStats.Gopher.read_hist[bin]++;
    }
    if (len < 0) {
	debug(50, 1) ("gopherReadReply: error reading: %s\n", xstrerror());
	if (ignoreErrno(errno)) {
	    commSetSelect(fd, COMM_SELECT_READ, gopherReadReply, data, 0);
	} else if (entry->mem_obj->inmem_hi == 0) {
	    ErrorState *err;
	    err = errorCon(ERR_READ_ERROR, HTTP_INTERNAL_SERVER_ERROR);
	    err->xerrno = errno;
	    err->url = xstrdup(storeUrl(entry));
	    errorAppendEntry(entry, err);
	    comm_close(fd);
	} else {
	    comm_close(fd);
	}
    } else if (len == 0 && entry->mem_obj->inmem_hi == 0) {
	ErrorState *err;
	err = errorCon(ERR_ZERO_SIZE_OBJECT, HTTP_SERVICE_UNAVAILABLE);
	err->xerrno = errno;
	err->url = xstrdup(gopherState->request);
	errorAppendEntry(entry, err);
	comm_close(fd);
    } else if (len == 0) {
	/* Connection closed; retrieval done. */
	/* flush the rest of data in temp buf if there is one. */
	if (gopherState->conversion != NORMAL)
	    gopherEndHTML(data);
	storeTimestampsSet(entry);
	storeBufferFlush(entry);
	fwdComplete(gopherState->fwdState);
	comm_close(fd);
    } else {
	if (gopherState->conversion != NORMAL) {
	    gopherToHTML(data, buf, len);
	} else {
	    storeAppend(entry, buf, len);
	}
	commSetSelect(fd,
	    COMM_SELECT_READ,
	    gopherReadReply,
	    data, 0);
    }
    memFree(buf, MEM_4K_BUF);
    return;
}

/* This will be called when request write is complete. Schedule read of
 * reply. */
static void
gopherSendComplete(int fd, char *buf, size_t size, int errflag, void *data)
{
    GopherStateData *gopherState = (GopherStateData *) data;
    StoreEntry *entry = gopherState->entry;
    debug(10, 5) ("gopherSendComplete: FD %d size: %d errflag: %d\n",
	fd, (int) size, errflag);
    if (size > 0) {
	fd_bytes(fd, size, FD_WRITE);
	kb_incr(&statCounter.server.all.kbytes_out, size);
	kb_incr(&statCounter.server.other.kbytes_out, size);
    }
    if (errflag) {
	ErrorState *err;
	err = errorCon(ERR_CONNECT_FAIL, HTTP_SERVICE_UNAVAILABLE);
	err->xerrno = errno;
	err->host = xstrdup(gopherState->req->host);
	err->port = gopherState->req->port;
	err->url = xstrdup(storeUrl(entry));
	errorAppendEntry(entry, err);
	comm_close(fd);
	if (buf)
	    memFree(buf, MEM_4K_BUF);	/* Allocated by gopherSendRequest. */
	return;
    }
    /* 
     * OK. We successfully reach remote site.  Start MIME typing
     * stuff.  Do it anyway even though request is not HTML type.
     */
    gopherMimeCreate(gopherState);
    switch (gopherState->type_id) {
    case GOPHER_DIRECTORY:
	/* we got to convert it first */
	storeBuffer(entry);
	gopherState->conversion = HTML_DIR;
	gopherState->HTML_header_added = 0;
	break;
    case GOPHER_INDEX:
	/* we got to convert it first */
	storeBuffer(entry);
	gopherState->conversion = HTML_INDEX_RESULT;
	gopherState->HTML_header_added = 0;
	break;
    case GOPHER_CSO:
	/* we got to convert it first */
	storeBuffer(entry);
	gopherState->conversion = HTML_CSO_RESULT;
	gopherState->cso_recno = 0;
	gopherState->HTML_header_added = 0;
	break;
    default:
	gopherState->conversion = NORMAL;
    }
    /* Schedule read reply. */
    commSetSelect(fd, COMM_SELECT_READ, gopherReadReply, gopherState, 0);
    commSetDefer(fd, fwdCheckDeferRead, entry);
    if (buf)
	memFree(buf, MEM_4K_BUF);	/* Allocated by gopherSendRequest. */
}

/* This will be called when connect completes. Write request. */
static void
gopherSendRequest(int fd, void *data)
{
    GopherStateData *gopherState = data;
    char *buf = memAllocate(MEM_4K_BUF);
    if (gopherState->type_id == GOPHER_CSO) {
	const char *t = strchr(gopherState->request, '?');
	if (t != NULL)
	    t++;		/* skip the ? */
	else
	    t = "";
	snprintf(buf, 4096, "query %s\r\nquit\r\n", t);
    } else if (gopherState->type_id == GOPHER_INDEX) {
	char *t = strchr(gopherState->request, '?');
	if (t != NULL)
	    *t = '\t';
	snprintf(buf, 4096, "%s\r\n", gopherState->request);
    } else {
	snprintf(buf, 4096, "%s\r\n", gopherState->request);
    }
    debug(10, 5) ("gopherSendRequest: FD %d\n", fd);
    comm_write(fd,
	buf,
	strlen(buf),
	gopherSendComplete,
	data,
	memFree4K);
    if (EBIT_TEST(gopherState->entry->flags, ENTRY_CACHABLE))
	storeSetPublicKey(gopherState->entry);	/* Make it public */
}

CBDATA_TYPE(GopherStateData);

void
gopherStart(FwdState * fwdState)
{
    int fd = fwdState->server_fd;
    StoreEntry *entry = fwdState->entry;
    GopherStateData *gopherState;
    CBDATA_INIT_TYPE(GopherStateData);
    gopherState = cbdataAlloc(GopherStateData);
    gopherState->buf = memAllocate(MEM_4K_BUF);
    storeLockObject(entry);
    gopherState->entry = entry;
    gopherState->fwdState = fwdState;
    debug(10, 3) ("gopherStart: %s\n", storeUrl(entry));
    statCounter.server.all.requests++;
    statCounter.server.other.requests++;
    /* Parse url. */
    gopher_request_parse(fwdState->request,
	&gopherState->type_id, gopherState->request);
#if OLD_PARSE_ERROR_CODE
    if (...) {
	ErrorState *err;
	err = errorCon(ERR_INVALID_URL, HTTP_BAD_REQUEST);
	err->url = xstrdup(storeUrl(entry));
	errorAppendEntry(entry, err);
	gopherStateFree(-1, gopherState);
	return;
    }
#endif
    comm_add_close_handler(fd, gopherStateFree, gopherState);
    if (((gopherState->type_id == GOPHER_INDEX) || (gopherState->type_id == GOPHER_CSO))
	&& (strchr(gopherState->request, '?') == NULL)) {
	/* Index URL without query word */
	/* We have to generate search page back to client. No need for connection */
	gopherMimeCreate(gopherState);
	if (gopherState->type_id == GOPHER_INDEX) {
	    gopherState->conversion = HTML_INDEX_PAGE;
	} else {
	    if (gopherState->type_id == GOPHER_CSO) {
		gopherState->conversion = HTML_CSO_PAGE;
	    } else {
		gopherState->conversion = HTML_INDEX_PAGE;
	    }
	}
	gopherToHTML(gopherState, (char *) NULL, 0);
	fwdComplete(fwdState);
	comm_close(fd);
	return;
    }
    gopherState->fd = fd;
    gopherState->fwdState = fwdState;
    commSetSelect(fd, COMM_SELECT_WRITE, gopherSendRequest, gopherState, 0);
    commSetTimeout(fd, Config.Timeout.read, gopherTimeout, gopherState);
}
