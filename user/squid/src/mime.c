
/*
 * $Id: mime.c,v 1.102.2.5 2005/03/26 02:50:53 hno Exp $
 *
 * DEBUG: section 25    MIME Parsing
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

#define GET_HDR_SZ 1024

typedef struct _mime_entry {
    char *pattern;
    regex_t compiled_pattern;
    char *icon;
    char *content_type;
    char *content_encoding;
    char transfer_mode;
    unsigned int view_option:1, download_option:1;
    struct _mime_entry *next;
} mimeEntry;

static mimeEntry *MimeTable = NULL;
static mimeEntry **MimeTableTail = &MimeTable;

static void mimeLoadIconFile(const char *icon);

/* returns a pointer to a field-value of the first matching field-name */
char *
mime_get_header(const char *mime, const char *name)
{
    return mime_get_header_field(mime, name, NULL);
}

/*
 * returns a pointer to a field-value of the first matching field-name where
 * field-value matches prefix if any
 */
char *
mime_get_header_field(const char *mime, const char *name, const char *prefix)
{
    LOCAL_ARRAY(char, header, GET_HDR_SZ);
    const char *p = NULL;
    char *q = NULL;
    char got = 0;
    const int namelen = name ? strlen(name) : 0;
    const int preflen = prefix ? strlen(prefix) : 0;
    int l;

    if (NULL == mime)
	return NULL;
    assert(NULL != name);

    debug(25, 5) ("mime_get_header: looking for '%s'\n", name);

    for (p = mime; *p; p += strcspn(p, "\n\r")) {
	if (strcmp(p, "\r\n\r\n") == 0 || strcmp(p, "\n\n") == 0)
	    return NULL;
	while (xisspace(*p))
	    p++;
	if (strncasecmp(p, name, namelen))
	    continue;
	if (!xisspace(p[namelen]) && p[namelen] != ':')
	    continue;
	l = strcspn(p, "\n\r") + 1;
	if (l > GET_HDR_SZ)
	    l = GET_HDR_SZ;
	xstrncpy(header, p, l);
	debug(25, 5) ("mime_get_header: checking '%s'\n", header);
	q = header;
	q += namelen;
	if (*q == ':')
	    q++, got = 1;
	while (xisspace(*q))
	    q++, got = 1;
	if (got && prefix) {
	    /* we could process list entries here if we had strcasestr(). */
	    /* make sure we did not match a part of another field-value */
	    got = !strncasecmp(q, prefix, preflen) && !xisalpha(q[preflen]);
	}
	if (got) {
	    debug(25, 5) ("mime_get_header: returning '%s'\n", q);
	    return q;
	}
    }
    return NULL;
}

size_t
headersEnd(const char *mime, size_t l)
{
    size_t e = 0;
    int state = 1;
    while (e < l && state < 3) {
	switch (state) {
	case 0:
	    if ('\n' == mime[e])
		state = 1;
	    break;
	case 1:
	    if ('\r' == mime[e])
		state = 2;
	    else if ('\n' == mime[e])
		state = 3;
	    else
		state = 0;
	    break;
	case 2:
	    if ('\n' == mime[e])
		state = 3;
	    else
		state = 0;
	    break;
	default:
	    break;
	}
	e++;
    }
    if (3 == state)
	return e;
    return 0;
}

const char *
mime_get_auth(const char *hdr, const char *auth_scheme, const char **auth_field)
{
    char *auth_hdr;
    char *t;
    if (auth_field)
	*auth_field = NULL;
    if (hdr == NULL)
	return NULL;
    if ((auth_hdr = mime_get_header(hdr, "Authorization")) == NULL)
	return NULL;
    if (auth_field)
	*auth_field = auth_hdr;
    if ((t = strtok(auth_hdr, " \t")) == NULL)
	return NULL;
    if (strcasecmp(t, auth_scheme) != 0)
	return NULL;
    if ((t = strtok(NULL, " \t")) == NULL)
	return NULL;
    return base64_decode(t);
}

static mimeEntry *
mimeGetEntry(const char *fn, int skip_encodings)
{
    mimeEntry *m;
    char *t;
    char *name = xstrdup(fn);
  try_again:
    for (m = MimeTable; m; m = m->next) {
	if (regexec(&m->compiled_pattern, name, 0, 0, 0) == 0)
	    break;
    }
    if (!skip_encodings)
	(void) 0;
    else if (m == NULL)
	(void) 0;
    else if (strcmp(m->content_type, dash_str))
	(void) 0;
    else if (!strcmp(m->content_encoding, dash_str))
	(void) 0;
    else {
	/* Assume we matched /\.\w$/ and cut off the last extension */
	if ((t = strrchr(name, '.'))) {
	    *t = '\0';
	    goto try_again;
	}
	/* What? A encoding without a extension? */
	m = NULL;
    }
    xfree(name);
    return m;
}

char *
mimeGetIcon(const char *fn)
{
    mimeEntry *m = mimeGetEntry(fn, 1);
    if (m == NULL)
	return NULL;
    if (!strcmp(m->icon, dash_str))
	return NULL;
    return m->icon;
}

const char *
mimeGetIconURL(const char *fn)
{
    static MemBuf mb = MemBufNULL;
    char *icon = mimeGetIcon(fn);
    if (icon == NULL)
	return null_string;
    if (Config.icons.use_short_names) {
	memBufReset(&mb);
	memBufPrintf(&mb, "/squid-internal-static/icons/%s", icon);
	return mb.buf;
    } else {
	return internalLocalUri("/squid-internal-static/icons/", icon);
    }
}

char *
mimeGetContentType(const char *fn)
{
    mimeEntry *m = mimeGetEntry(fn, 1);
    if (m == NULL)
	return NULL;
    if (!strcmp(m->content_type, dash_str))
	return NULL;
    return m->content_type;
}

char *
mimeGetContentEncoding(const char *fn)
{
    mimeEntry *m = mimeGetEntry(fn, 0);
    if (m == NULL)
	return NULL;
    if (!strcmp(m->content_encoding, dash_str))
	return NULL;
    return m->content_encoding;
}

char
mimeGetTransferMode(const char *fn)
{
    mimeEntry *m = mimeGetEntry(fn, 0);
    return m ? m->transfer_mode : 'I';
}

int
mimeGetDownloadOption(const char *fn)
{
    mimeEntry *m = mimeGetEntry(fn, 1);
    return m ? m->download_option : 0;
}

int
mimeGetViewOption(const char *fn)
{
    mimeEntry *m = mimeGetEntry(fn, 0);
    return m ? m->view_option : 0;
}

/* Initializes/reloads the mime table
 * Note: Due to Solaris STDIO problems the caller should NOT
 * call mimeFreeMemory on reconfigure. This way, if STDIO
 * fails we at least have the old copy loaded.
 */
void
mimeInit(char *filename)
{
    FILE *fp;
    char buf[BUFSIZ];
    char chopbuf[BUFSIZ];
    char *t;
    char *pattern;
    char *icon;
    char *type;
    char *encoding;
    char *mode;
    char *option;
    int view_option;
    int download_option;
    regex_t re;
    mimeEntry *m;
    int re_flags = REG_EXTENDED | REG_NOSUB | REG_ICASE;
    if (filename == NULL)
	return;
    if ((fp = fopen(filename, "r")) == NULL) {
	debug(25, 1) ("mimeInit: %s: %s\n", filename, xstrerror());
	return;
    }
#if defined (_SQUID_CYGWIN_)
    setmode(fileno(fp), O_TEXT);
#endif
    mimeFreeMemory();
    while (fgets(buf, BUFSIZ, fp)) {
	if ((t = strchr(buf, '#')))
	    *t = '\0';
	if ((t = strchr(buf, '\r')))
	    *t = '\0';
	if ((t = strchr(buf, '\n')))
	    *t = '\0';
	if (buf[0] == '\0')
	    continue;
	xstrncpy(chopbuf, buf, BUFSIZ);
	if ((pattern = strtok(chopbuf, w_space)) == NULL) {
	    debug(25, 1) ("mimeInit: parse error: '%s'\n", buf);
	    continue;
	}
	if ((type = strtok(NULL, w_space)) == NULL) {
	    debug(25, 1) ("mimeInit: parse error: '%s'\n", buf);
	    continue;
	}
	if ((icon = strtok(NULL, w_space)) == NULL) {
	    debug(25, 1) ("mimeInit: parse error: '%s'\n", buf);
	    continue;
	}
	if ((encoding = strtok(NULL, w_space)) == NULL) {
	    debug(25, 1) ("mimeInit: parse error: '%s'\n", buf);
	    continue;
	}
	if ((mode = strtok(NULL, w_space)) == NULL) {
	    debug(25, 1) ("mimeInit: parse error: '%s'\n", buf);
	    continue;
	}
	download_option = 0;
	view_option = 0;
	while ((option = strtok(NULL, w_space)) != NULL) {
	    if (!strcmp(option, "+download"))
		download_option = 1;
	    else if (!strcmp(option, "+view"))
		view_option = 1;
	    else
		debug(25, 1) ("mimeInit: unknown option: '%s' (%s)\n", buf, option);
	}
	if (regcomp(&re, pattern, re_flags) != 0) {
	    debug(25, 1) ("mimeInit: regcomp error: '%s'\n", buf);
	    continue;
	}
	m = xcalloc(1, sizeof(mimeEntry));
	m->pattern = xstrdup(pattern);
	m->content_type = xstrdup(type);
	m->icon = xstrdup(icon);
	m->content_encoding = xstrdup(encoding);
	m->compiled_pattern = re;
	if (!strcasecmp(mode, "ascii"))
	    m->transfer_mode = 'A';
	else if (!strcasecmp(mode, "text"))
	    m->transfer_mode = 'A';
	else
	    m->transfer_mode = 'I';
	m->view_option = view_option;
	m->download_option = download_option;
	*MimeTableTail = m;
	MimeTableTail = &m->next;
	debug(25, 5) ("mimeInit: added '%s'\n", buf);
    }
    fclose(fp);
    /*
     * Create Icon StoreEntry's
     */
    for (m = MimeTable; m != NULL; m = m->next)
	mimeLoadIconFile(m->icon);
    debug(25, 1) ("Loaded Icons.\n");
}

void
mimeFreeMemory(void)
{
    mimeEntry *m;
    while ((m = MimeTable)) {
	MimeTable = m->next;
	safe_free(m->pattern);
	safe_free(m->content_type);
	safe_free(m->icon);
	safe_free(m->content_encoding);
	regfree(&m->compiled_pattern);
	safe_free(m);
    }
    MimeTableTail = &MimeTable;
}

static void
mimeLoadIconFile(const char *icon)
{
    int fd;
    int n;
    request_flags flags;
    struct stat sb;
    StoreEntry *e;
    LOCAL_ARRAY(char, path, MAXPATHLEN);
    LOCAL_ARRAY(char, url, MAX_URL);
    char *buf;
    const char *type = mimeGetContentType(icon);
    HttpReply *reply;
    http_version_t version;
    request_t *r;
    if (type == NULL)
	fatal("Unknown icon format while reading mime.conf\n");
    buf = internalLocalUri("/squid-internal-static/icons/", icon);
    xstrncpy(url, buf, MAX_URL);
    if (storeGetPublic(url, METHOD_GET))
	return;
    snprintf(path, MAXPATHLEN, "%s/%s", Config.icons.directory, icon);
    fd = file_open(path, O_RDONLY | O_BINARY);
    if (fd < 0) {
	debug(25, 0) ("mimeLoadIconFile: %s: %s\n", path, xstrerror());
	return;
    }
    if (fstat(fd, &sb) < 0) {
	debug(25, 0) ("mimeLoadIconFile: FD %d: fstat: %s\n", fd, xstrerror());
	file_close(fd);
	return;
    }
    flags = null_request_flags;
    flags.cachable = 1;
    e = storeCreateEntry(url,
	url,
	flags,
	METHOD_GET);
    assert(e != NULL);
    EBIT_SET(e->flags, ENTRY_SPECIAL);
    storeSetPublicKey(e);
    storeBuffer(e);
    r = urlParse(METHOD_GET, url);
    if (NULL == r)
	fatal("mimeLoadIcon: cannot parse internal URL");
    e->mem_obj->request = requestLink(r);
    httpReplyReset(reply = e->mem_obj->reply);
    httpBuildVersion(&version, 1, 0);
    httpReplySetHeaders(reply, version, HTTP_OK, NULL,
	type, sb.st_size, sb.st_mtime, -1);
    reply->cache_control = httpHdrCcCreate();
    httpHdrCcSetMaxAge(reply->cache_control, 86400);
    httpHeaderPutCc(&reply->header, reply->cache_control);
    httpReplySwapOut(reply, e);
    reply->hdr_sz = e->mem_obj->inmem_hi;	/* yuk */
    /* read the file into the buffer and append it to store */
    buf = memAllocate(MEM_4K_BUF);
    while ((n = FD_READ_METHOD(fd, buf, 4096)) > 0)
	storeAppend(e, buf, n);
    file_close(fd);
    storeBufferFlush(e);
    storeComplete(e);
    storeTimestampsSet(e);
    debug(25, 3) ("Loaded icon %s\n", url);
    storeUnlockObject(e);
    memFree(buf, MEM_4K_BUF);
}
