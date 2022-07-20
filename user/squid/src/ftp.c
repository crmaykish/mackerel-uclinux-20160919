
/*
 * $Id: ftp.c,v 1.316.2.25 2005/03/26 02:50:53 hno Exp $
 *
 * DEBUG: section 9     File Transfer Protocol (FTP)
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

static const char *const crlf = "\r\n";
static char cbuf[1024];

typedef enum {
    BEGIN,
    SENT_USER,
    SENT_PASS,
    SENT_TYPE,
    SENT_MDTM,
    SENT_SIZE,
    SENT_PORT,
    SENT_PASV,
    SENT_CWD,
    SENT_LIST,
    SENT_NLST,
    SENT_REST,
    SENT_RETR,
    SENT_STOR,
    SENT_QUIT,
    READING_DATA,
    WRITING_DATA,
    SENT_MKDIR
} ftp_state_t;

struct _ftp_flags {
    unsigned int isdir:1;
    unsigned int pasv_supported:1;
    unsigned int skip_whitespace:1;
    unsigned int rest_supported:1;
    unsigned int pasv_only:1;
    unsigned int authenticated:1;
    unsigned int http_header_sent:1;
    unsigned int tried_nlst:1;
    unsigned int use_base:1;
    unsigned int root_dir:1;
    unsigned int no_dotdot:1;
    unsigned int html_header_sent:1;
    unsigned int binary:1;
    unsigned int try_slash_hack:1;
    unsigned int put:1;
    unsigned int put_mkdir:1;
    unsigned int listformat_unknown:1;
};

typedef struct _Ftpdata {
    StoreEntry *entry;
    request_t *request;
    char user[MAX_URL];
    char password[MAX_URL];
    int password_url;
    char *reply_hdr;
    int reply_hdr_state;
    String title_url;
    String base_href;
    int conn_att;
    int login_att;
    ftp_state_t state;
    time_t mdtm;
    squid_off_t size;
    wordlist *pathcomps;
    char *filepath;
    squid_off_t restart_offset;
    squid_off_t restarted_offset;
    int rest_att;
    char *proxy_host;
    int list_width;
    wordlist *cwd_message;
    char *old_request;
    char *old_reply;
    char *old_filepath;
    char typecode;
    struct {
	int fd;
	char *buf;
	size_t size;
	size_t offset;
	FREE *freefunc;
	wordlist *message;
	char *last_command;
	char *last_reply;
	int replycode;
    } ctrl;
    struct {
	int fd;
	char *buf;
	size_t size;
	size_t offset;
	FREE *freefunc;
	char *host;
	u_short port;
    } data;
    struct _ftp_flags flags;
    FwdState *fwd;
} FtpStateData;

typedef struct {
    char type;
    squid_off_t size;
    char *date;
    char *name;
    char *showname;
    char *link;
} ftpListParts;

typedef void (FTPSM) (FtpStateData *);

#define FTP_LOGIN_ESCAPED 1
#define FTP_LOGIN_NOT_ESCAPED 0

/* Local functions */
static CNCB ftpPasvCallback;
static PF ftpDataRead;
static PF ftpDataWrite;
static CWCB ftpDataWriteCallback;
static PF ftpStateFree;
static PF ftpTimeout;
static PF ftpReadControlReply;
static CWCB ftpWriteCommandCallback;
static void ftpLoginParser(const char *, FtpStateData *, int escaped);
static wordlist *ftpParseControlReply(char *, size_t, int *, int *);
static int ftpRestartable(FtpStateData * ftpState);
static void ftpAppendSuccessHeader(FtpStateData * ftpState);
static void ftpAuthRequired(HttpReply * reply, request_t * request, const char *realm);
static void ftpHackShortcut(FtpStateData * ftpState, FTPSM * nextState);
static void ftpUnhack(FtpStateData * ftpState);
static void ftpScheduleReadControlReply(FtpStateData *, int);
static void ftpHandleControlReply(FtpStateData *);
static char *ftpHtmlifyListEntry(const char *line, FtpStateData * ftpState);
static void ftpFailed(FtpStateData *, err_type);
static void ftpFailedErrorMessage(FtpStateData *, err_type);

/*
 * State machine functions
 * send == state transition
 * read == wait for response, and select next state transition
 * other == Transition logic
 */
static FTPSM ftpReadWelcome;
static FTPSM ftpSendUser;
static FTPSM ftpReadUser;
static FTPSM ftpSendPass;
static FTPSM ftpReadPass;
static FTPSM ftpSendType;
static FTPSM ftpReadType;
static FTPSM ftpSendMdtm;
static FTPSM ftpReadMdtm;
static FTPSM ftpSendSize;
static FTPSM ftpReadSize;
static FTPSM ftpSendPort;
static FTPSM ftpReadPort;
static FTPSM ftpSendPasv;
static FTPSM ftpReadPasv;
static FTPSM ftpTraverseDirectory;
static FTPSM ftpListDir;
static FTPSM ftpGetFile;
static FTPSM ftpSendCwd;
static FTPSM ftpReadCwd;
static FTPSM ftpRestOrList;
static FTPSM ftpSendList;
static FTPSM ftpSendNlst;
static FTPSM ftpReadList;
static FTPSM ftpSendRest;
static FTPSM ftpReadRest;
static FTPSM ftpSendRetr;
static FTPSM ftpReadRetr;
static FTPSM ftpReadTransferDone;
static FTPSM ftpSendStor;
static FTPSM ftpReadStor;
static FTPSM ftpWriteTransferDone;
static FTPSM ftpSendReply;
static FTPSM ftpSendMkdir;
static FTPSM ftpReadMkdir;
static FTPSM ftpFail;
static FTPSM ftpSendQuit;
static FTPSM ftpReadQuit;
/************************************************
** State Machine Description (excluding hacks) **
*************************************************
From			To
---------------------------------------
Welcome			User
User			Pass
Pass			Type
Type			TraverseDirectory / GetFile
TraverseDirectory	Cwd / GetFile / ListDir
Cwd			TraverseDirectory / Mkdir
GetFile			Mdtm
Mdtm			Size
Size			Pasv
ListDir			Pasv
Pasv			FileOrList
FileOrList		Rest / Retr / Nlst / List / Mkdir (PUT /xxx;type=d)
Rest			Retr
Retr / Nlst / List	DataRead* (on datachannel)
DataRead*		ReadTransferDone
ReadTransferDone	DataTransferDone
Stor			DataWrite* (on datachannel)
DataWrite*		RequestPutBody** (from client)
RequestPutBody**	DataWrite* / WriteTransferDone
WriteTransferDone	DataTransferDone
DataTransferDone	Quit
Quit			-
************************************************/

FTPSM *FTP_SM_FUNCS[] =
{
    ftpReadWelcome,		/* BEGIN */
    ftpReadUser,		/* SENT_USER */
    ftpReadPass,		/* SENT_PASS */
    ftpReadType,		/* SENT_TYPE */
    ftpReadMdtm,		/* SENT_MDTM */
    ftpReadSize,		/* SENT_SIZE */
    ftpReadPort,		/* SENT_PORT */
    ftpReadPasv,		/* SENT_PASV */
    ftpReadCwd,			/* SENT_CWD */
    ftpReadList,		/* SENT_LIST */
    ftpReadList,		/* SENT_NLST */
    ftpReadRest,		/* SENT_REST */
    ftpReadRetr,		/* SENT_RETR */
    ftpReadStor,		/* SENT_STOR */
    ftpReadQuit,		/* SENT_QUIT */
    ftpReadTransferDone,	/* READING_DATA (RETR,LIST,NLST) */
    ftpWriteTransferDone,	/* WRITING_DATA (STOR) */
    ftpReadMkdir		/* SENT_MKDIR */
};

static void
ftpStateFree(int fdnotused, void *data)
{
    FtpStateData *ftpState = data;
    ftpState->ctrl.fd = -1;
    if (ftpState->data.fd > -1) {
	int fd = ftpState->data.fd;
	ftpState->data.fd = -1;
	comm_close(fd);
    }
    cbdataFree(ftpState);
}

static void
ftpStateFreed(void *data)
{
    FtpStateData *ftpState = data;
    if (ftpState == NULL)
	return;
    debug(9, 3) ("ftpStateFree: %s\n", storeUrl(ftpState->entry));
    storeUnregisterAbort(ftpState->entry);
    storeUnlockObject(ftpState->entry);
    if (ftpState->reply_hdr) {
	memFree(ftpState->reply_hdr, MEM_8K_BUF);
	/* this seems unnecessary, but people report SEGV's
	 * when freeing memory in this function */
	ftpState->reply_hdr = NULL;
    }
    requestUnlink(ftpState->request);
    if (ftpState->ctrl.buf) {
	ftpState->ctrl.freefunc(ftpState->ctrl.buf);
	/* this seems unnecessary, but people report SEGV's
	 * when freeing memory in this function */
	ftpState->ctrl.buf = NULL;
    }
    if (ftpState->data.buf) {
	ftpState->data.freefunc(ftpState->data.buf);
	/* this seems unnecessary, but people report SEGV's
	 * when freeing memory in this function */
	ftpState->data.buf = NULL;
    }
    if (ftpState->pathcomps)
	wordlistDestroy(&ftpState->pathcomps);
    if (ftpState->ctrl.message)
	wordlistDestroy(&ftpState->ctrl.message);
    if (ftpState->cwd_message)
	wordlistDestroy(&ftpState->cwd_message);
    safe_free(ftpState->ctrl.last_reply);
    safe_free(ftpState->ctrl.last_command);
    safe_free(ftpState->old_request);
    safe_free(ftpState->old_reply);
    safe_free(ftpState->old_filepath);
    stringClean(&ftpState->title_url);
    stringClean(&ftpState->base_href);
    safe_free(ftpState->filepath);
    safe_free(ftpState->data.host);
}

static void
ftpLoginParser(const char *login, FtpStateData * ftpState, int escaped)
{
    char *s = NULL;
    xstrncpy(ftpState->user, login, MAX_URL);
    if ((s = strchr(ftpState->user, ':'))) {
	*s = 0;
	xstrncpy(ftpState->password, s + 1, MAX_URL);
	if (escaped) {
	    rfc1738_unescape(ftpState->password);
	    ftpState->password_url = 1;
	}
    } else {
	xstrncpy(ftpState->password, null_string, MAX_URL);
    }
    if (escaped)
	rfc1738_unescape(ftpState->user);
    if (!ftpState->user[0])
	xstrncpy(ftpState->user, "anonymous", MAX_URL);
    if (strcmp(ftpState->user, "anonymous") == 0 && !ftpState->password[0])
	xstrncpy(ftpState->password, Config.Ftp.anon_user, MAX_URL);
}

static void
ftpTimeout(int fd, void *data)
{
    FtpStateData *ftpState = data;
    StoreEntry *entry = ftpState->entry;
    debug(9, 4) ("ftpTimeout: FD %d: '%s'\n", fd, storeUrl(entry));
    if (SENT_PASV == ftpState->state && fd == ftpState->data.fd) {
	/* stupid ftp.netscape.com */
	ftpState->fwd->flags.dont_retry = 0;
	ftpState->fwd->flags.ftp_pasv_failed = 1;
	debug(9, 1) ("ftpTimeout: timeout in SENT_PASV state\n");
    }
    ftpFailed(ftpState, ERR_READ_TIMEOUT);
    /* ftpFailed closes ctrl.fd and frees ftpState */
}

static void
ftpListingStart(FtpStateData * ftpState)
{
    StoreEntry *e = ftpState->entry;
    wordlist *w;
    char *dirup;
    int i, j, k;
    char *title;
    storeBuffer(e);
    storeAppendPrintf(e, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
    storeAppendPrintf(e, "<!-- HTML listing generated by Squid %s -->\n",
	version_string);
    storeAppendPrintf(e, "<!-- %s -->\n", mkrfc1123(squid_curtime));
    storeAppendPrintf(e, "<HTML><HEAD><TITLE>\n");
    storeAppendPrintf(e, "FTP Directory: %s\n",
	html_quote(strBuf(ftpState->title_url)));
    storeAppendPrintf(e, "</TITLE>\n");
    storeAppendPrintf(e, "<STYLE type=\"text/css\"><!--BODY{background-color:#ffffff;font-family:verdana,sans-serif}--></STYLE>\n");
    if (ftpState->flags.use_base)
	storeAppendPrintf(e, "<BASE HREF=\"%s\">\n",
	    html_quote(strBuf(ftpState->base_href)));
    storeAppendPrintf(e, "</HEAD><BODY>\n");
    if (ftpState->cwd_message) {
	storeAppendPrintf(e, "<PRE>\n");
	for (w = ftpState->cwd_message; w; w = w->next)
	    storeAppendPrintf(e, "%s\n", html_quote(w->key));
	storeAppendPrintf(e, "</PRE>\n");
	storeAppendPrintf(e, "<HR noshade size=\"1px\">\n");
	wordlistDestroy(&ftpState->cwd_message);
    }
    storeAppendPrintf(e, "<H2>\n");
    storeAppendPrintf(e, "FTP Directory: ");
    /* "ftp://" == 6 characters */
    assert(strLen(ftpState->title_url) >= 6);
    title = html_quote(strBuf(ftpState->title_url));
    for (i = 6, j = 0; title[i]; j = i) {
	storeAppendPrintf(e, "<A HREF=\"");
	i += strcspn(&title[i], "/");
	if (title[i] == '/')
	    i++;
	for (k = 0; k < i; k++)
	    storeAppendPrintf(e, "%c", title[k]);
	storeAppendPrintf(e, "\">");
	for (k = j; k < i - 1; k++)
	    storeAppendPrintf(e, "%c", title[k]);
	if (strBuf(ftpState->title_url)[k] != '/')
	    storeAppendPrintf(e, "%c", title[k++]);
	storeAppendPrintf(e, "</A>");
	if (k < i)
	    storeAppendPrintf(e, "%c", title[k++]);
	if (i == j) {
	    /* Error guard, or "assert" */
	    storeAppendPrintf(e, "ERROR: Failed to parse URL: %s\n",
		html_quote(strBuf(ftpState->title_url)));
	    debug(9, 0) ("Failed to parse URL: %s\n", strBuf(ftpState->title_url));
	    break;
	}
    }
    storeAppendPrintf(e, "</H2>\n");
    storeAppendPrintf(e, "<PRE>\n");
    dirup = ftpHtmlifyListEntry("<internal-dirup>", ftpState);
    storeAppend(e, dirup, strlen(dirup));
    storeBufferFlush(e);
    ftpState->flags.html_header_sent = 1;
}

static void
ftpListingFinish(FtpStateData * ftpState)
{
    StoreEntry *e = ftpState->entry;
    storeBuffer(e);
    storeAppendPrintf(e, "</PRE>\n");
    if (ftpState->flags.listformat_unknown && !ftpState->flags.tried_nlst) {
	storeAppendPrintf(e, "<A HREF=\"./;type=d\">[As plain directory]</A>\n");
    } else if (ftpState->typecode == 'D') {
	const char *path = ftpState->filepath ? ftpState->filepath : ".";
	storeAppendPrintf(e, "<A HREF=\"%s/\">[As extended directory]</A>\n", html_quote(path));
    }
    storeAppendPrintf(e, "<HR noshade size=\"1px\">\n");
    storeAppendPrintf(e, "<ADDRESS>\n");
    storeAppendPrintf(e, "Generated %s by %s (%s)\n",
	mkrfc1123(squid_curtime),
	getMyHostname(),
	full_appname_string);
    storeAppendPrintf(e, "</ADDRESS></BODY></HTML>\n");
    storeBufferFlush(e);
}

static const char *Month[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int
is_month(const char *buf)
{
    int i;
    for (i = 0; i < 12; i++)
	if (!strcasecmp(buf, Month[i]))
	    return 1;
    return 0;
}


static void
ftpListPartsFree(ftpListParts ** parts)
{
    safe_free((*parts)->date);
    safe_free((*parts)->name);
    safe_free((*parts)->showname);
    safe_free((*parts)->link);
    safe_free(*parts);
}

#define MAX_TOKENS 64

static ftpListParts *
ftpListParseParts(const char *buf, struct _ftp_flags flags)
{
    ftpListParts *p = NULL;
    char *t = NULL;
    const char *ct = NULL;
    char *tokens[MAX_TOKENS];
    int i;
    int n_tokens;
    static char tbuf[128];
    char *xbuf = NULL;
    static int scan_ftp_initialized = 0;
    static regex_t scan_ftp_integer;
    static regex_t scan_ftp_time;
    static regex_t scan_ftp_dostime;
    static regex_t scan_ftp_dosdate;

    if (!scan_ftp_initialized) {
	scan_ftp_initialized = 1;
	regcomp(&scan_ftp_integer, "^[0123456789]+$", REG_EXTENDED | REG_NOSUB);
	regcomp(&scan_ftp_time, "^[0123456789:]+$", REG_EXTENDED | REG_NOSUB);
	regcomp(&scan_ftp_dosdate, "^[0123456789]+-[0123456789]+-[0123456789]+$", REG_EXTENDED | REG_NOSUB);
	regcomp(&scan_ftp_dostime, "^[0123456789]+:[0123456789]+[AP]M$", REG_EXTENDED | REG_NOSUB | REG_ICASE);
    }
    if (buf == NULL)
	return NULL;
    if (*buf == '\0')
	return NULL;
    p = xcalloc(1, sizeof(ftpListParts));
    n_tokens = 0;
    memset(tokens, 0, sizeof(tokens));
    xbuf = xstrdup(buf);
    if (flags.tried_nlst) {
	/* Machine readable format, one name per line */
	p->name = xbuf;
	p->type = '\0';
	return p;
    }
    for (t = strtok(xbuf, w_space); t && n_tokens < MAX_TOKENS; t = strtok(NULL, w_space))
	tokens[n_tokens++] = xstrdup(t);
    xfree(xbuf);
    /* locate the Month field */
    for (i = 3; i < n_tokens - 2; i++) {
	char *size = tokens[i - 1];
	char *month = tokens[i];
	char *day = tokens[i + 1];
	char *year = tokens[i + 2];
	if (!is_month(month))
	    continue;
	if (regexec(&scan_ftp_integer, size, 0, NULL, 0) != 0)
	    continue;
	if (regexec(&scan_ftp_integer, day, 0, NULL, 0) != 0)
	    continue;
	if (regexec(&scan_ftp_time, day, 0, NULL, 0) != 0)	/* Yr | hh:mm */
	    continue;
	snprintf(tbuf, 128, "%s %2s %5s",
	    month, day, year);
	if (!strstr(buf, tbuf))
	    snprintf(tbuf, 128, "%s %2s %-5s",
		month, day, year);
	if ((t = strstr(buf, tbuf))) {
	    p->type = *tokens[0];
	    p->size = strto_off_t(size, NULL, 10);
	    p->date = xstrdup(tbuf);
	    if (flags.skip_whitespace) {
		t += strlen(tbuf);
		while (strchr(w_space, *t))
		    t++;
	    } else {
		/* XXX assumes a single space between date and filename
		 * suggested by:  Nathan.Bailey@cc.monash.edu.au and
		 * Mike Battersby <mike@starbug.bofh.asn.au> */
		t += strlen(tbuf) + 1;
	    }
	    p->name = xstrdup(t);
	    if ((t = strstr(p->name, " -> "))) {
		*t = '\0';
		p->link = xstrdup(t + 4);
	    }
	    goto found;
	}
	break;
    }
    /* try it as a DOS listing, 04-05-70 09:33PM ... */
    if (n_tokens > 3 &&
	regexec(&scan_ftp_dosdate, tokens[0], 0, NULL, 0) == 0 &&
	regexec(&scan_ftp_dostime, tokens[1], 0, NULL, 0) == 0) {
	if (!strcasecmp(tokens[2], "<dir>")) {
	    p->type = 'd';
	} else {
	    p->type = '-';
	    p->size = strto_off_t(tokens[2], NULL, 10);
	}
	snprintf(tbuf, 128, "%s %s", tokens[0], tokens[1]);
	p->date = xstrdup(tbuf);
	if (p->type == 'd') {
	    /* Directory.. name begins with first printable after <dir> */
	    ct = strstr(buf, tokens[2]);
	    ct += strlen(tokens[2]);
	    while (xisspace(*ct))
		ct++;
	    if (!*ct)
		ct = NULL;
	} else {
	    /* A file. Name begins after size, with a space in between */
	    snprintf(tbuf, 128, " %s %s", tokens[2], tokens[3]);
	    ct = strstr(buf, tbuf);
	    if (ct) {
		ct += strlen(tokens[2]) + 2;
	    }
	}
	p->name = xstrdup(ct ? ct : tokens[3]);
	goto found;
    }
    /* Try EPLF format; carson@lehman.com */
    if (buf[0] == '+') {
	ct = buf + 1;
	p->type = 0;
	while (ct && *ct) {
	    time_t t;
	    int l = strcspn(ct, ",");
	    char *tmp;
	    if (l < 1)
		goto blank;
	    switch (*ct) {
	    case '\t':
		p->name = xstrndup(ct + 1, l + 1);
		break;
	    case 's':
		p->size = strto_off_t(ct + 1, NULL, 10);
		break;
	    case 'm':
		t = (time_t) strto_off_t(ct + 1, &tmp, 0);
		if (tmp != ct + l)
		    break;	/* not a valid integer */
		p->date = xstrdup(ctime(&t));
		*(strstr(p->date, "\n")) = '\0';
		break;
	    case '/':
		p->type = 'd';
		break;
	    case 'r':
		p->type = '-';
		break;
	    case 'i':
		break;
	    default:
		break;
	    }
	  blank:
	    ct = strstr(ct, ",");
	    if (ct) {
		ct++;
	    }
	}
	if (p->type == 0) {
	    p->type = '-';
	}
	if (p->name)
	    goto found;
	else
	    safe_free(p->date);
    }
  found:
    for (i = 0; i < n_tokens; i++)
	xfree(tokens[i]);
    if (!p->name)
	ftpListPartsFree(&p);	/* cleanup */
    return p;
}

static const char *
dots_fill(size_t len)
{
    static char buf[256];
    int i = 0;
    if (len > Config.Ftp.list_width) {
	memset(buf, ' ', 256);
	buf[0] = '\n';
	buf[Config.Ftp.list_width + 4] = '\0';
	return buf;
    }
    for (i = (int) len; i < Config.Ftp.list_width; i++)
	buf[i - len] = (i % 2) ? '.' : ' ';
    buf[i - len] = '\0';
    return buf;
}

static char *
ftpHtmlifyListEntry(const char *line, FtpStateData * ftpState)
{
    LOCAL_ARRAY(char, icon, 2048);
    LOCAL_ARRAY(char, href, 2048 + 40);
    LOCAL_ARRAY(char, text, 2048);
    LOCAL_ARRAY(char, size, 2048);
    LOCAL_ARRAY(char, chdir, 2048 + 40);
    LOCAL_ARRAY(char, view, 2048 + 40);
    LOCAL_ARRAY(char, download, 2048 + 40);
    LOCAL_ARRAY(char, link, 2048 + 40);
    LOCAL_ARRAY(char, html, 8192);
    size_t width = Config.Ftp.list_width;
    ftpListParts *parts;
    *icon = *href = *text = *size = *chdir = *view = *download = *link = *html = '\0';
    if ((int) strlen(line) > 1024) {
	snprintf(html, 8192, "%s\n", line);
	return html;
    }
    /* Handle builtin <dirup> */
    if (strcmp(line, "<internal-dirup>") == 0) {
	/* <A HREF="{href}">{icon}</A> <A HREF="{href}">{text}</A> {link} */
	snprintf(icon, 2048, "<IMG border=\"0\" SRC=\"%s\" ALT=\"%-6s\">",
	    mimeGetIconURL("internal-dirup"),
	    "[DIRUP]");
	if (!ftpState->flags.no_dotdot && !ftpState->flags.root_dir) {
	    /* Normal directory */
	    strcpy(href, "../");
	    strcpy(text, "Parent Directory");
	} else if (!ftpState->flags.no_dotdot && ftpState->flags.root_dir) {
	    /* "Top level" directory */
	    strcpy(href, "%2e%2e/");
	    strcpy(text, "Parent Directory");
	    snprintf(link, 2048, "(<A HREF=\"%s\">%s</A>)",
		"%2f/",
		"Root Directory");
	} else if (ftpState->flags.no_dotdot && !ftpState->flags.root_dir) {
	    /* Normal directory where last component is / or ..  */
	    strcpy(href, "%2e%2e/");
	    strcpy(text, "Parent Directory");
	    snprintf(link, 2048, "(<A HREF=\"%s\">%s</A>)",
		"../",
		"Back");
	} else {		/* NO_DOTDOT && ROOT_DIR */
	    /* "UNIX Root" directory */
	    strcpy(href, "/");
	    strcpy(text, "Home Directory");
	}
	snprintf(html, 8192, "<A HREF=\"%s\">%s</A> <A HREF=\"%s\">%s</A> %s\n",
	    href, icon, href, text, link);
	return html;
    }
    if ((parts = ftpListParseParts(line, ftpState->flags)) == NULL) {
	const char *p;
	snprintf(html, 8192, "%s\n", line);
	for (p = line; *p && xisspace(*p); p++);
	if (*p && !xisspace(*p))
	    ftpState->flags.listformat_unknown = 1;
	return html;
    }
    if (!strcmp(parts->name, ".") || !strcmp(parts->name, "..")) {
	*html = '\0';
	ftpListPartsFree(&parts);
	return html;
    }
    parts->showname = xstrdup(parts->name);
    if (!Config.Ftp.list_wrap) {
	if (strlen(parts->showname) > width - 1) {
	    *(parts->showname + width - 1) = '>';
	    *(parts->showname + width - 0) = '\0';
	}
    }
    /* {icon} {text} . . . {date}{size}{chdir}{view}{download}{link}\n  */
    xstrncpy(href, rfc1738_escape_part(parts->name), 2048);
    xstrncpy(text, parts->showname, 2048);
    switch (parts->type) {
    case 'd':
	snprintf(icon, 2048, "<IMG border=\"0\" SRC=\"%s\" ALT=\"%-6s\">",
	    mimeGetIconURL("internal-dir"),
	    "[DIR]");
	strcat(href, "/");	/* margin is allocated above */
	break;
    case 'l':
	snprintf(icon, 2048, "<IMG border=\"0\" SRC=\"%s\" ALT=\"%-6s\">",
	    mimeGetIconURL("internal-link"),
	    "[LINK]");
	/* sometimes there is an 'l' flag, but no "->" link */
	if (parts->link) {
	    char *link2 = xstrdup(html_quote(rfc1738_escape(parts->link)));
	    snprintf(link, 2048, " -> <A HREF=\"%s\">%s</A>",
		link2,
		html_quote(parts->link));
	    safe_free(link2);
	}
	break;
    case '\0':
	snprintf(icon, 2048, "<IMG border=\"0\" SRC=\"%s\" ALT=\"%-6s\">",
	    mimeGetIconURL(parts->name),
	    "[UNKNOWN]");
	snprintf(chdir, 2048, " <A HREF=\"%s/;type=d\"><IMG border=\"0\" SRC=\"%s\" "
	    "ALT=\"[DIR]\"></A>",
	    rfc1738_escape_part(parts->name),
	    mimeGetIconURL("internal-dir"));
	break;
    case '-':
    default:
	{
	    squid_off_t sz = parts->size;
	    char units = ' ';
	    snprintf(icon, 2048, "<IMG border=\"0\" SRC=\"%s\" ALT=\"%-6s\">",
		mimeGetIconURL(parts->name),
		"[FILE]");
	    if (sz > 10 * 1024) {
		sz = (sz + 1023) >> 10;
		units = 'K';
	    }
	    if (sz > 10 * 1024) {
		sz = (sz + 1023) >> 10;
		units = 'M';
	    }
	    if (sz > 10 * 1024) {
		sz = (sz + 1023) >> 10;
		units = 'G';
	    }
	    snprintf(size, 2048, " %6" PRINTF_OFF_T "%c", sz, units);
	}
	break;
    }
    if (parts->type != 'd') {
	if (mimeGetViewOption(parts->name)) {
	    snprintf(view, 2048, " <A HREF=\"%s;type=a\"><IMG border=\"0\" SRC=\"%s\" "
		"ALT=\"[VIEW]\"></A>",
		href, mimeGetIconURL("internal-view"));
	}
	if (mimeGetDownloadOption(parts->name)) {
	    snprintf(download, 2048, " <A HREF=\"%s;type=i\"><IMG border=\"0\" SRC=\"%s\" "
		"ALT=\"[DOWNLOAD]\"></A>",
		href, mimeGetIconURL("internal-download"));
	}
    }
    /* <A HREF="{href}">{icon}</A> <A HREF="{href}">{text}</A> . . . {date}{size}{chdir}{view}{download}{link}\n  */
    if (parts->type != '\0') {
	snprintf(html, 8192, "<A HREF=\"%s\">%s</A> <A HREF=\"%s\">%s</A>%s "
	    "%s%8s%s%s%s%s\n",
	    href, icon, href, html_quote(text), dots_fill(strlen(text)),
	    parts->date, size, chdir, view, download, link);
    } else {
	/* Plain listing. {icon} {text} ... {chdir}{view}{download} */
	snprintf(html, 8192, "<A HREF=\"%s\">%s</A> <A HREF=\"%s\">%s</A>%s "
	    "%s%s%s%s\n",
	    href, icon, href, html_quote(text), dots_fill(strlen(text)),
	    chdir, view, download, link);
    }
    ftpListPartsFree(&parts);
    return html;
}

static void
ftpParseListing(FtpStateData * ftpState)
{
    char *buf = ftpState->data.buf;
    char *sbuf;			/* NULL-terminated copy of buf */
    char *end;
    char *line;
    char *s;
    char *t;
    size_t linelen;
    size_t usable;
    StoreEntry *e = ftpState->entry;
    size_t len = ftpState->data.offset;
    /*
     * We need a NULL-terminated buffer for scanning, ick
     */
    sbuf = xmalloc(len + 1);
    xstrncpy(sbuf, buf, len + 1);
    end = sbuf + len - 1;
    while (*end != '\r' && *end != '\n' && end > sbuf)
	end--;
    usable = end - sbuf;
    debug(9, 3) ("ftpParseListing: usable = %d\n", (int) usable);
    if (usable == 0) {
	debug(9, 3) ("ftpParseListing: didn't find end for %s\n", storeUrl(e));
	xfree(sbuf);
	return;
    }
    debug(9, 3) ("ftpParseListing: %d bytes to play with\n", (int) len);
    line = memAllocate(MEM_4K_BUF);
    end++;
    storeBuffer(e);
    s = sbuf;
    s += strspn(s, crlf);
    for (; s < end; s += strcspn(s, crlf), s += strspn(s, crlf)) {
	debug(9, 3) ("ftpParseListing: s = {%s}\n", s);
	linelen = strcspn(s, crlf) + 1;
	if (linelen < 2)
	    break;
	if (linelen > 4096)
	    linelen = 4096;
	xstrncpy(line, s, linelen);
	debug(9, 7) ("ftpParseListing: {%s}\n", line);
	if (!strncmp(line, "total", 5))
	    continue;
	t = ftpHtmlifyListEntry(line, ftpState);
	assert(t != NULL);
	storeAppend(e, t, strlen(t));
    }
    storeBufferFlush(e);
    assert(usable <= len);
    if (usable < len) {
	/* must copy partial line to beginning of buf */
	linelen = len - usable;
	if (linelen > 4096)
	    linelen = 4096;
	xstrncpy(line, end, linelen);
	xstrncpy(ftpState->data.buf, line, ftpState->data.size);
	ftpState->data.offset = strlen(ftpState->data.buf);
    }
    memFree(line, MEM_4K_BUF);
    xfree(sbuf);
}

static void
ftpDataComplete(FtpStateData * ftpState)
{
    debug(9, 3) ("ftpDataComplete\n");
    /* Connection closed; transfer done. */
    if (ftpState->data.fd > -1) {
	/*
	 * close data socket so it does not occupy resources while
	 * we wait
	 */
	comm_close(ftpState->data.fd);
	ftpState->data.fd = -1;
    }
    /* expect the "transfer complete" message on the control socket */
    ftpScheduleReadControlReply(ftpState, 1);
}

static void
ftpDataRead(int fd, void *data)
{
    FtpStateData *ftpState = data;
    int len;
    int j;
    int bin;
    StoreEntry *entry = ftpState->entry;
    size_t read_sz;
#if DELAY_POOLS
    MemObject *mem = entry->mem_obj;
    delay_id delay_id = delayMostBytesAllowed(mem);
#endif
    assert(fd == ftpState->data.fd);
    if (EBIT_TEST(entry->flags, ENTRY_ABORTED)) {
	comm_close(ftpState->ctrl.fd);
	return;
    }
    errno = 0;
    read_sz = ftpState->data.size - ftpState->data.offset;
#if DELAY_POOLS
    read_sz = delayBytesWanted(delay_id, 1, read_sz);
#endif
    memset(ftpState->data.buf + ftpState->data.offset, '\0', read_sz);
    statCounter.syscalls.sock.reads++;
    len = FD_READ_METHOD(fd, ftpState->data.buf + ftpState->data.offset, read_sz);
    if (len > 0) {
	fd_bytes(fd, len, FD_READ);
#if DELAY_POOLS
	delayBytesIn(delay_id, len);
#endif
	kb_incr(&statCounter.server.all.kbytes_in, len);
	kb_incr(&statCounter.server.ftp.kbytes_in, len);
	ftpState->data.offset += len;
    }
    debug(9, 5) ("ftpDataRead: FD %d, Read %d bytes\n", fd, len);
    if (len > 0) {
	IOStats.Ftp.reads++;
	for (j = len - 1, bin = 0; j; bin++)
	    j >>= 1;
	IOStats.Ftp.read_hist[bin]++;
    }
    if (!ftpState->flags.http_header_sent && len >= 0) {
	ftpAppendSuccessHeader(ftpState);
	if (ftpState->flags.isdir)
	    ftpListingStart(ftpState);
    }
    if (len < 0) {
	debug(50, ignoreErrno(errno) ? 3 : 1) ("ftpDataRead: read error: %s\n", xstrerror());
	if (ignoreErrno(errno)) {
	    commSetSelect(fd,
		COMM_SELECT_READ,
		ftpDataRead,
		data,
		Config.Timeout.read);
	} else {
	    if (!ftpState->flags.http_header_sent && !ftpState->fwd->flags.ftp_pasv_failed && ftpState->flags.pasv_supported) {
		ftpState->fwd->flags.dont_retry = 0;	/* this is a retryable error */
		ftpState->fwd->flags.ftp_pasv_failed = 1;
	    }
	    ftpFailed(ftpState, ERR_READ_ERROR);
	    /* ftpFailed closes ctrl.fd and frees ftpState */
	    return;
	}
    } else if (len == 0) {
	ftpDataComplete(ftpState);
    } else {
	if (ftpState->flags.isdir) {
	    ftpParseListing(ftpState);
	} else {
	    storeAppend(entry, ftpState->data.buf, len);
	    ftpState->data.offset = 0;
	}
	commSetSelect(fd,
	    COMM_SELECT_READ,
	    ftpDataRead,
	    data,
	    Config.Timeout.read);
    }
}

/*
 * ftpCheckAuth
 *
 * Return 1 if we have everything needed to complete this request.
 * Return 0 if something is missing.
 */
static int
ftpCheckAuth(FtpStateData * ftpState, const HttpHeader * req_hdr)
{
    char *orig_user;
    const char *auth;
    ftpLoginParser(ftpState->request->login, ftpState, FTP_LOGIN_ESCAPED);
    if (!ftpState->user[0])
	return 1;		/* no name */
    if (ftpState->password_url || ftpState->password[0])
	return 1;		/* passwd provided in URL */
    /* URL has name, but no passwd */
    if (!(auth = httpHeaderGetAuth(req_hdr, HDR_AUTHORIZATION, "Basic")))
	return 0;		/* need auth header */
    ftpState->flags.authenticated = 1;
    orig_user = xstrdup(ftpState->user);
    ftpLoginParser(auth, ftpState, FTP_LOGIN_NOT_ESCAPED);
    if (strcmp(orig_user, ftpState->user) == 0) {
	xfree(orig_user);
	return 1;		/* same username */
    }
    xstrncpy(ftpState->user, orig_user, sizeof(ftpState->user));
    xfree(orig_user);
    return 0;			/* different username */
}

static void
ftpCheckUrlpath(FtpStateData * ftpState)
{
    request_t *request = ftpState->request;
    int l;
    const char *t;
    if ((t = strRChr(request->urlpath, ';')) != NULL) {
	if (strncasecmp(t + 1, "type=", 5) == 0) {
	    ftpState->typecode = (char) toupper((int) *(t + 6));
	    strCutPtr(request->urlpath, t);
	}
    }
    l = strLen(request->urlpath);
    /* check for null path */
    if (!l) {
	ftpState->flags.isdir = 1;
	ftpState->flags.root_dir = 1;
	ftpState->flags.use_base = 1;	/* Work around broken browsers */
    } else if (!strCmp(request->urlpath, "/%2f/")) {
	/* UNIX root directory */
	ftpState->flags.isdir = 1;
	ftpState->flags.root_dir = 1;
    } else if ((l >= 1) && (*(strBuf(request->urlpath) + l - 1) == '/')) {
	/* Directory URL, ending in / */
	ftpState->flags.isdir = 1;
	if (l == 1)
	    ftpState->flags.root_dir = 1;
    }
}

static void
ftpBuildTitleUrl(FtpStateData * ftpState)
{
    request_t *request = ftpState->request;

    stringReset(&ftpState->title_url, "ftp://");
    if (strcmp(ftpState->user, "anonymous")) {
	strCat(ftpState->title_url, ftpState->user);
	strCat(ftpState->title_url, "@");
    }
    strCat(ftpState->title_url, request->host);
    if (request->port != urlDefaultPort(PROTO_FTP)) {
	strCat(ftpState->title_url, ":");
	strCat(ftpState->title_url, xitoa(request->port));
    }
    strCat(ftpState->title_url, strBuf(request->urlpath));
    {
	char *t = xstrdup(strBuf(ftpState->title_url));
	rfc1738_unescape(t);
	stringReset(&ftpState->title_url, t);
	xfree(t);
    }

    stringReset(&ftpState->base_href, "ftp://");
    if (strcmp(ftpState->user, "anonymous")) {
	strCat(ftpState->base_href, rfc1738_escape_part(ftpState->user));
	if (ftpState->password_url) {
	    strCat(ftpState->base_href, ":");
	    strCat(ftpState->base_href, rfc1738_escape_part(ftpState->password));
	}
	strCat(ftpState->base_href, "@");
    }
    strCat(ftpState->base_href, request->host);
    if (request->port != urlDefaultPort(PROTO_FTP)) {
	strCat(ftpState->base_href, ":");
	strCat(ftpState->base_href, xitoa(request->port));
    }
    strCat(ftpState->base_href, strBuf(request->urlpath));
    strCat(ftpState->base_href, "/");
}

CBDATA_TYPE(FtpStateData);
void
ftpStart(FwdState * fwd)
{
    request_t *request = fwd->request;
    StoreEntry *entry = fwd->entry;
    int fd = fwd->server_fd;
    LOCAL_ARRAY(char, realm, 8192);
    const char *url = storeUrl(entry);
    FtpStateData *ftpState;
    HttpReply *reply;

    CBDATA_INIT_TYPE_FREECB(FtpStateData, ftpStateFreed);
    ftpState = cbdataAlloc(FtpStateData);
    debug(9, 3) ("ftpStart: '%s'\n", url);
    statCounter.server.all.requests++;
    statCounter.server.ftp.requests++;
    storeLockObject(entry);
    ftpState->entry = entry;
    ftpState->request = requestLink(request);
    ftpState->ctrl.fd = fd;
    ftpState->data.fd = -1;
    ftpState->size = -1;
    ftpState->mdtm = -1;
    if (Config.Ftp.passive && !fwd->flags.ftp_pasv_failed)
	ftpState->flags.pasv_supported = 1;
    ftpState->flags.rest_supported = 1;
    ftpState->fwd = fwd;
    comm_add_close_handler(fd, ftpStateFree, ftpState);
    if (ftpState->request->method == METHOD_PUT)
	ftpState->flags.put = 1;
    if (!ftpCheckAuth(ftpState, &request->header)) {
	/* This request is not fully authenticated */
	if (request->port == 21) {
	    snprintf(realm, 8192, "ftp %s", ftpState->user);
	} else {
	    snprintf(realm, 8192, "ftp %s port %d",
		ftpState->user, request->port);
	}
	/* create reply */
	reply = entry->mem_obj->reply;
	assert(reply != NULL);
	/* create appropriate reply */
	ftpAuthRequired(reply, request, realm);
	httpReplySwapOut(reply, entry);
	fwdComplete(ftpState->fwd);
	comm_close(fd);
	return;
    }
    ftpCheckUrlpath(ftpState);
    ftpBuildTitleUrl(ftpState);
    debug(9, 5) ("ftpStart: host=%s, path=%s, user=%s, passwd=%s\n",
	ftpState->request->host, strBuf(ftpState->request->urlpath),
	ftpState->user, ftpState->password);
    ftpState->state = BEGIN;
    ftpState->ctrl.last_command = xstrdup("Connect to server");
    ftpState->ctrl.buf = memAllocate(MEM_4K_BUF);
    ftpState->ctrl.freefunc = memFree4K;
    ftpState->ctrl.size = 4096;
    ftpState->ctrl.offset = 0;
    ftpState->data.buf = xmalloc(SQUID_TCP_SO_RCVBUF);
    ftpState->data.size = SQUID_TCP_SO_RCVBUF;
    ftpState->data.freefunc = xfree;
    ftpScheduleReadControlReply(ftpState, 0);
}

/* ====================================================================== */

/* escapes any IAC (0xFF) characters. Returns a new string */
static char *
escapeIAC(const char *buf)
{
    int n;
    char *ret;
    unsigned const char *p;
    unsigned char *r;
    for (p = (unsigned const char *) buf, n = 1; *p; n++, p++)
	if (*p == 255)
	    n++;
    ret = xmalloc(n);
    for (p = (unsigned const char *) buf, r = (unsigned char *) ret; *p; p++) {
	*r++ = *p;
	if (*p == 255)
	    *r++ = 255;
    }
    *r++ = '\0';
    assert((r - (unsigned char *) ret) == n);
    return ret;
}

/* removes any telnet options. Same string returned */
static char *
decodeTelnet(char *buf)
{
    char *p = buf;
    while ((p = strstr(p, "\377\377")) != NULL) {
	p++;
	memmove(p, p + 1, strlen(p + 1) + 1);
    }
    return buf;
}

static void
ftpWriteCommand(const char *buf, FtpStateData * ftpState)
{
    char *ebuf;
    debug(9, 5) ("ftpWriteCommand: %s\n", buf);
    if (Config.Ftp.telnet)
	ebuf = escapeIAC(buf);
    else
	ebuf = xstrdup(buf);
    safe_free(ftpState->ctrl.last_command);
    safe_free(ftpState->ctrl.last_reply);
    ftpState->ctrl.last_command = xstrdup(buf);
    comm_write(ftpState->ctrl.fd,
	ebuf,
	strlen(ebuf),
	ftpWriteCommandCallback,
	ftpState,
	xfree);
    ftpScheduleReadControlReply(ftpState, 0);
}

static void
ftpWriteCommandCallback(int fd, char *bufnotused, size_t size, int errflag, void *data)
{
    FtpStateData *ftpState = data;
    debug(9, 7) ("ftpWriteCommandCallback: wrote %d bytes\n", (int) size);
    if (size > 0) {
	fd_bytes(fd, size, FD_WRITE);
	kb_incr(&statCounter.server.all.kbytes_out, size);
	kb_incr(&statCounter.server.ftp.kbytes_out, size);
    }
    if (errflag == COMM_ERR_CLOSING)
	return;
    if (errflag) {
	debug(9, 1) ("ftpWriteCommandCallback: FD %d: %s\n", fd, xstrerror());
	ftpFailed(ftpState, ERR_WRITE_ERROR);
	/* ftpFailed closes ctrl.fd and frees ftpState */
	return;
    }
}

static wordlist *
ftpParseControlReply(char *buf, size_t len, int *codep, int *used)
{
    char *s;
    char *sbuf;
    char *end;
    int usable;
    int complete = 0;
    wordlist *head = NULL;
    wordlist *list;
    wordlist **tail = &head;
    int offset;
    int linelen;
    int code = -1;
    debug(9, 5) ("ftpParseControlReply\n");
    /*
     * We need a NULL-terminated buffer for scanning, ick
     */
    sbuf = xmalloc(len + 1);
    xstrncpy(sbuf, buf, len + 1);
    end = sbuf + len - 1;
    while (*end != '\r' && *end != '\n' && end > sbuf)
	end--;
    usable = end - sbuf;
    debug(9, 3) ("ftpParseControlReply: usable = %d\n", usable);
    if (usable == 0) {
	debug(9, 3) ("ftpParseControlReply: didn't find end of line\n");
	safe_free(sbuf);
	return NULL;
    }
    debug(9, 3) ("ftpParseControlReply: %d bytes to play with\n", (int) len);
    end++;
    s = sbuf;
    s += strspn(s, crlf);
    for (; s < end; s += strcspn(s, crlf), s += strspn(s, crlf)) {
	if (complete)
	    break;
	debug(9, 3) ("ftpParseControlReply: s = {%s}\n", s);
	linelen = strcspn(s, crlf) + 1;
	if (linelen < 2)
	    break;
	if (linelen > 3)
	    complete = (*s >= '0' && *s <= '9' && *(s + 3) == ' ');
	if (complete)
	    code = atoi(s);
	offset = 0;
	if (linelen > 3)
	    if (*s >= '0' && *s <= '9' && (*(s + 3) == '-' || *(s + 3) == ' '))
		offset = 4;
	list = memAllocate(MEM_WORDLIST);
	list->key = xmalloc(linelen - offset);
	xstrncpy(list->key, s + offset, linelen - offset);
	if (Config.Ftp.telnet)
	    decodeTelnet(list->key);
	debug(9, 7) ("%d %s\n", code, list->key);
	*tail = list;
	tail = &list->next;
    }
    *used = (int) (s - sbuf);
    safe_free(sbuf);
    if (!complete)
	wordlistDestroy(&head);
    if (codep)
	*codep = code;
    return head;
}

static void
ftpScheduleReadControlReply(FtpStateData * ftpState, int buffered_ok)
{
    debug(9, 3) ("ftpScheduleReadControlReply: FD %d\n", ftpState->ctrl.fd);
    if (buffered_ok && ftpState->ctrl.offset > 0) {
	/* We've already read some reply data */
	ftpHandleControlReply(ftpState);
    } else {
	commSetSelect(ftpState->ctrl.fd,
	    COMM_SELECT_READ,
	    ftpReadControlReply,
	    ftpState,
	    Config.Timeout.read);
	/*
	 * Cancel the timeout on the Data socket (if any) and
	 * establish one on the control socket.
	 */
	if (ftpState->data.fd > -1)
	    commSetTimeout(ftpState->data.fd, -1, NULL, NULL);
	commSetTimeout(ftpState->ctrl.fd, Config.Timeout.read, ftpTimeout,
	    ftpState);
    }
}

static void
ftpReadControlReply(int fd, void *data)
{
    FtpStateData *ftpState = data;
    StoreEntry *entry = ftpState->entry;
    int len;
    debug(9, 5) ("ftpReadControlReply\n");
    if (EBIT_TEST(entry->flags, ENTRY_ABORTED)) {
	comm_close(ftpState->ctrl.fd);
	return;
    }
    assert(ftpState->ctrl.offset < ftpState->ctrl.size);
    statCounter.syscalls.sock.reads++;
    len = FD_READ_METHOD(fd,
	ftpState->ctrl.buf + ftpState->ctrl.offset,
	ftpState->ctrl.size - ftpState->ctrl.offset);
    if (len > 0) {
	fd_bytes(fd, len, FD_READ);
	kb_incr(&statCounter.server.all.kbytes_in, len);
	kb_incr(&statCounter.server.ftp.kbytes_in, len);
    }
    debug(9, 5) ("ftpReadControlReply: FD %d, Read %d bytes\n", fd, len);
    if (len < 0) {
	debug(50, ignoreErrno(errno) ? 3 : 1) ("ftpReadControlReply: read error: %s\n", xstrerror());
	if (ignoreErrno(errno)) {
	    ftpScheduleReadControlReply(ftpState, 0);
	} else {
	    ftpFailed(ftpState, ERR_READ_ERROR);
	    /* ftpFailed closes ctrl.fd and frees ftpState */
	    return;
	}
	return;
    }
    if (len == 0) {
	if (entry->store_status == STORE_PENDING) {
	    ftpFailed(ftpState, ERR_FTP_FAILURE);
	    /* ftpFailed closes ctrl.fd and frees ftpState */
	    return;
	}
	comm_close(ftpState->ctrl.fd);
	return;
    }
    len += ftpState->ctrl.offset;
    ftpState->ctrl.offset = len;
    assert(len <= ftpState->ctrl.size);
    ftpHandleControlReply(ftpState);
}

static void
ftpHandleControlReply(FtpStateData * ftpState)
{
    char *oldbuf;
    wordlist **W;
    int bytes_used = 0;
    wordlistDestroy(&ftpState->ctrl.message);
    ftpState->ctrl.message = ftpParseControlReply(ftpState->ctrl.buf,
	ftpState->ctrl.offset, &ftpState->ctrl.replycode, &bytes_used);
    if (ftpState->ctrl.message == NULL) {
	/* didn't get complete reply yet */
	if (ftpState->ctrl.offset == ftpState->ctrl.size) {
	    oldbuf = ftpState->ctrl.buf;
	    ftpState->ctrl.buf = xcalloc(ftpState->ctrl.size << 1, 1);
	    xmemcpy(ftpState->ctrl.buf, oldbuf, ftpState->ctrl.size);
	    ftpState->ctrl.size <<= 1;
	    ftpState->ctrl.freefunc(oldbuf);
	    ftpState->ctrl.freefunc = xfree;
	}
	ftpScheduleReadControlReply(ftpState, 0);
	return;
    } else if (ftpState->ctrl.offset == bytes_used) {
	/* used it all up */
	ftpState->ctrl.offset = 0;
    } else {
	/* Got some data past the complete reply */
	assert(bytes_used < ftpState->ctrl.offset);
	ftpState->ctrl.offset -= bytes_used;
	xmemmove(ftpState->ctrl.buf, ftpState->ctrl.buf + bytes_used,
	    ftpState->ctrl.offset);
    }
    /* Move the last line of the reply message to ctrl.last_reply */
    for (W = &ftpState->ctrl.message; (*W)->next; W = &(*W)->next);
    safe_free(ftpState->ctrl.last_reply);
    ftpState->ctrl.last_reply = xstrdup((*W)->key);
    wordlistDestroy(W);
    /* Copy the rest of the message to cwd_message to be printed in
     * error messages
     */
    wordlistAddWl(&ftpState->cwd_message, ftpState->ctrl.message);
    debug(9, 8) ("ftpHandleControlReply: state=%d, code=%d\n", ftpState->state,
	ftpState->ctrl.replycode);
    FTP_SM_FUNCS[ftpState->state] (ftpState);
}

/* ====================================================================== */

static void
ftpReadWelcome(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("ftpReadWelcome\n");
    if (ftpState->flags.pasv_only)
	ftpState->login_att++;
    /* Dont retry if the FTP server accepted the connection */
    ftpState->fwd->flags.dont_retry = 1;
    if (code == 220) {
	if (ftpState->ctrl.message) {
	    if (strstr(ftpState->ctrl.message->key, "NetWare"))
		ftpState->flags.skip_whitespace = 1;
	}
	ftpSendUser(ftpState);
    } else if (code == 120) {
	if (NULL != ftpState->ctrl.message)
	    debug(9, 3) ("FTP server is busy: %s\n",
		ftpState->ctrl.message->key);
	return;
    } else {
	ftpFail(ftpState);
    }
}

static void
ftpSendUser(FtpStateData * ftpState)
{
    if (ftpState->proxy_host != NULL)
	snprintf(cbuf, 1024, "USER %s@%s\r\n",
	    ftpState->user,
	    ftpState->request->host);
    else
	snprintf(cbuf, 1024, "USER %s\r\n", ftpState->user);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_USER;
}

static void
ftpReadUser(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("ftpReadUser\n");
    if (code == 230) {
	ftpReadPass(ftpState);
    } else if (code == 331) {
	ftpSendPass(ftpState);
    } else {
	ftpFail(ftpState);
    }
}

static void
ftpSendPass(FtpStateData * ftpState)
{
    snprintf(cbuf, 1024, "PASS %s\r\n", ftpState->password);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_PASS;
}

static void
ftpReadPass(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("ftpReadPass\n");
    if (code == 230) {
	ftpSendType(ftpState);
    } else {
	ftpFail(ftpState);
    }
}

static void
ftpSendType(FtpStateData * ftpState)
{
    const char *t;
    const char *filename;
    char mode;
    /*
     * Ref section 3.2.2 of RFC 1738
     */
    switch (mode = ftpState->typecode) {
    case 'D':
	mode = 'A';
	break;
    case 'A':
    case 'I':
	break;
    default:
	if (ftpState->flags.isdir) {
	    mode = 'A';
	} else {
	    t = strRChr(ftpState->request->urlpath, '/');
	    filename = t ? t + 1 : strBuf(ftpState->request->urlpath);
	    mode = mimeGetTransferMode(filename);
	}
	break;
    }
    if (mode == 'I')
	ftpState->flags.binary = 1;
    else
	ftpState->flags.binary = 0;
    snprintf(cbuf, 1024, "TYPE %c\r\n", mode);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_TYPE;
}

static void
ftpReadType(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    char *path;
    char *d, *p;
    debug(9, 3) ("This is ftpReadType\n");
    if (code == 200) {
	p = path = xstrdup(strBuf(ftpState->request->urlpath));
	if (*p == '/')
	    p++;
	while (*p) {
	    d = p;
	    p += strcspn(p, "/");
	    if (*p)
		*p++ = '\0';
	    rfc1738_unescape(d);
	    if (*d)
		wordlistAdd(&ftpState->pathcomps, d);
	}
	xfree(path);
	if (ftpState->pathcomps)
	    ftpTraverseDirectory(ftpState);
	else
	    ftpListDir(ftpState);
    } else {
	ftpFail(ftpState);
    }
}

static void
ftpTraverseDirectory(FtpStateData * ftpState)
{
    wordlist *w;
    debug(9, 4) ("ftpTraverseDirectory %s\n",
	ftpState->filepath ? ftpState->filepath : "<NULL>");

    safe_free(ftpState->filepath);
    /* Done? */
    if (ftpState->pathcomps == NULL) {
	debug(9, 3) ("the final component was a directory\n");
	ftpListDir(ftpState);
	return;
    }
    /* Go to next path component */
    w = ftpState->pathcomps;
    ftpState->filepath = w->key;
    ftpState->pathcomps = w->next;
    memFree(w, MEM_WORDLIST);
    /* Check if we are to CWD or RETR */
    if (ftpState->pathcomps != NULL || ftpState->flags.isdir) {
	ftpSendCwd(ftpState);
    } else {
	debug(9, 3) ("final component is probably a file\n");
	ftpGetFile(ftpState);
	return;
    }
}

static void
ftpSendCwd(FtpStateData * ftpState)
{
    char *path = ftpState->filepath;
    debug(9, 3) ("ftpSendCwd\n");
    if (!strcmp(path, "..") || !strcmp(path, "/")) {
	ftpState->flags.no_dotdot = 1;
    } else {
	ftpState->flags.no_dotdot = 0;
    }
    snprintf(cbuf, 1024, "CWD %s\r\n", path);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_CWD;
}

static void
ftpReadCwd(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadCwd\n");
    if (code >= 200 && code < 300) {
	/* CWD OK */
	ftpUnhack(ftpState);
	/* Reset cwd_message to only include the last message */
	if (ftpState->cwd_message)
	    wordlistDestroy(&ftpState->cwd_message);
	ftpState->cwd_message = ftpState->ctrl.message;
	ftpState->ctrl.message = NULL;
	/* Continue to traverse the path */
	ftpTraverseDirectory(ftpState);
    } else {
	/* CWD FAILED */
	if (!ftpState->flags.put)
	    ftpFail(ftpState);
	else
	    ftpSendMkdir(ftpState);
    }
}

static void
ftpSendMkdir(FtpStateData * ftpState)
{
    char *path = ftpState->filepath;
    debug(9, 3) ("ftpSendMkdir: with path=%s\n", path);
    snprintf(cbuf, 1024, "MKD %s\r\n", path);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_MKDIR;
}

static void
ftpReadMkdir(FtpStateData * ftpState)
{
    char *path = ftpState->filepath;
    int code = ftpState->ctrl.replycode;

    debug(9, 3) ("ftpReadMkdir: path %s, code %d\n", path, code);
    if (code == 257) {		/* success */
	ftpSendCwd(ftpState);
    } else if (code == 550) {	/* dir exists */
	if (ftpState->flags.put_mkdir) {
	    ftpState->flags.put_mkdir = 1;
	    ftpSendCwd(ftpState);
	} else
	    ftpSendReply(ftpState);
    } else
	ftpSendReply(ftpState);
}

static void
ftpGetFile(FtpStateData * ftpState)
{
    assert(*ftpState->filepath != '\0');
    ftpState->flags.isdir = 0;
    ftpSendMdtm(ftpState);
}

static void
ftpListDir(FtpStateData * ftpState)
{
    if (!ftpState->flags.isdir) {
	debug(9, 3) ("Directory path did not end in /\n");
	strCat(ftpState->title_url, "/");
	ftpState->flags.isdir = 1;
	ftpState->flags.use_base = 1;
    }
    ftpSendPasv(ftpState);
}

static void
ftpSendMdtm(FtpStateData * ftpState)
{
    assert(*ftpState->filepath != '\0');
    snprintf(cbuf, 1024, "MDTM %s\r\n", ftpState->filepath);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_MDTM;
}

static void
ftpReadMdtm(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadMdtm\n");
    if (code == 213) {
	ftpState->mdtm = parse_iso3307_time(ftpState->ctrl.last_reply);
	ftpUnhack(ftpState);
    } else if (code < 0) {
	ftpFail(ftpState);
    }
    ftpSendSize(ftpState);
}

static void
ftpSendSize(FtpStateData * ftpState)
{
    /* Only send SIZE for binary transfers. The returned size
     * is useless on ASCII transfers */
    if (ftpState->flags.binary) {
	assert(ftpState->filepath != NULL);
	assert(*ftpState->filepath != '\0');
	snprintf(cbuf, 1024, "SIZE %s\r\n", ftpState->filepath);
	ftpWriteCommand(cbuf, ftpState);
	ftpState->state = SENT_SIZE;
    } else
	/* Skip to next state no non-binary transfers */
	ftpSendPasv(ftpState);
}

static void
ftpReadSize(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadSize\n");
    if (code == 213) {
	ftpUnhack(ftpState);
	ftpState->size = strto_off_t(ftpState->ctrl.last_reply, NULL, 10);
	if (ftpState->size == 0) {
	    debug(9, 2) ("ftpReadSize: SIZE reported %s on %s\n",
		ftpState->ctrl.last_reply,
		strBuf(ftpState->title_url));
	    ftpState->size = -1;
	}
    } else if (code < 0) {
	ftpFail(ftpState);
    }
    ftpSendPasv(ftpState);
}

static void
ftpSendPasv(FtpStateData * ftpState)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    if (ftpState->request->method == METHOD_HEAD) {
	/* Terminate here for HEAD requests */
	ftpAppendSuccessHeader(ftpState);
	storeTimestampsSet(ftpState->entry);
	/*
	 * On rare occasions I'm seeing the entry get aborted after
	 * ftpReadControlReply() and before here, probably when
	 * trying to write to the client.
	 */
	if (!EBIT_TEST(ftpState->entry->flags, ENTRY_ABORTED))
	    fwdComplete(ftpState->fwd);
	ftpSendQuit(ftpState);
	return;
    }
    if (ftpState->data.fd >= 0) {
	/* Close old connection */
	comm_close(ftpState->data.fd);
	ftpState->data.fd = -1;
    }
    if (!ftpState->flags.pasv_supported) {
	ftpSendPort(ftpState);
	return;
    }
    addr_len = sizeof(addr);
    if (getsockname(ftpState->ctrl.fd, (struct sockaddr *) &addr, &addr_len)) {
	debug(9, 0) ("ftpSendPasv: getsockname(%d,..): %s\n",
	    ftpState->ctrl.fd, xstrerror());
	ftpFail(ftpState);
	return;
    }
    /* Open data channel with the same local address as control channel */
    fd = comm_open(SOCK_STREAM,
	0,
	addr.sin_addr,
	0,
	COMM_NONBLOCKING,
	storeUrl(ftpState->entry));
    debug(9, 3) ("ftpSendPasv: Unconnected data socket created on FD %d\n", fd);
    if (fd < 0) {
	ftpFail(ftpState);
	return;
    }
    /*
     * No comm_add_close_handler() here.  If we have both ctrl and
     * data FD's call ftpStateFree() upon close, then we have
     * to delete the close handler which did NOT get called
     * to prevent ftpStateFree() getting called twice.
     * Instead we'll always call comm_close() on the ctrl FD.
     *
     * XXX this should not actually matter if the ftpState is cbdata
     * managed correctly and comm close handlers are cbdata fenced
     */
    ftpState->data.fd = fd;
    snprintf(cbuf, 1024, "PASV\r\n");
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_PASV;
    /*
     * ugly hack for ftp servers like ftp.netscape.com that sometimes
     * dont acknowledge PASV commands.
     */
    commSetTimeout(ftpState->data.fd, 15, ftpTimeout, ftpState);
}

static void
ftpReadPasv(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    int h1, h2, h3, h4;
    int p1, p2;
    int n;
    u_short port;
    int fd = ftpState->data.fd;
    char *buf;
    LOCAL_ARRAY(char, ipaddr, 1024);
    debug(9, 3) ("This is ftpReadPasv\n");
    if (code != 227) {
	debug(9, 3) ("PASV not supported by remote end\n");
	ftpSendPort(ftpState);
	return;
    }
    /*  227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).  */
    /*  ANSI sez [^0-9] is undefined, it breaks on Watcom cc */
    debug(9, 5) ("scanning: %s\n", ftpState->ctrl.last_reply);
    buf = ftpState->ctrl.last_reply + strcspn(ftpState->ctrl.last_reply, "0123456789");
    n = sscanf(buf, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    if (n != 6 || p1 < 0 || p2 < 0 || p1 > 255 || p2 > 255) {
	debug(9, 1) ("Odd PASV reply from %s: %s\n", fd_table[ftpState->ctrl.fd].ipaddr, ftpState->ctrl.last_reply);
	ftpSendPort(ftpState);
	return;
    }
    snprintf(ipaddr, 1024, "%d.%d.%d.%d", h1, h2, h3, h4);
    if (!safe_inet_addr(ipaddr, NULL)) {
	debug(9, 1) ("Unsafe PASV reply from %s: %s\n", fd_table[ftpState->ctrl.fd].ipaddr, ftpState->ctrl.last_reply);
	ftpSendPort(ftpState);
	return;
    }
    port = ((p1 << 8) + p2);
    if (0 == port) {
	debug(9, 1) ("Unsafe PASV reply from %s: %s\n", fd_table[ftpState->ctrl.fd].ipaddr, ftpState->ctrl.last_reply);
	ftpSendPort(ftpState);
	return;
    }
    if (Config.Ftp.sanitycheck) {
	if (port < 1024) {
	    debug(9, 1) ("Unsafe PASV reply from %s: %s\n", fd_table[ftpState->ctrl.fd].ipaddr, ftpState->ctrl.last_reply);
	    ftpSendPort(ftpState);
	    return;
	}
    }
    ftpState->data.port = port;
    if (Config.Ftp.sanitycheck)
	ftpState->data.host = xstrdup(fd_table[ftpState->ctrl.fd].ipaddr);
    else
	ftpState->data.host = xstrdup(ipaddr);
    safe_free(ftpState->ctrl.last_command);
    safe_free(ftpState->ctrl.last_reply);
    ftpState->ctrl.last_command = xstrdup("Connect to server data port");
    debug(9, 5) ("ftpReadPasv: connecting to %s, port %d\n", ftpState->data.host, ftpState->data.port);
    commConnectStart(fd, ipaddr, port, ftpPasvCallback, ftpState);
}

static void
ftpPasvCallback(int fd, int status, void *data)
{
    FtpStateData *ftpState = data;
    debug(9, 3) ("ftpPasvCallback\n");
    if (status != COMM_OK) {
	debug(9, 2) ("ftpPasvCallback: failed to connect. Retrying without PASV.\n");
	ftpState->fwd->flags.dont_retry = 0;	/* this is a retryable error */
	ftpState->fwd->flags.ftp_pasv_failed = 1;
	ftpFailed(ftpState, ERR_NONE);
	/* ftpFailed closes ctrl.fd and frees ftpState */
	return;
    }
    ftpRestOrList(ftpState);
}

static int
ftpOpenListenSocket(FtpStateData * ftpState, int fallback)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int on = 1;
    u_short port = 0;
    /*
     * Tear down any old data connection if any. We are about to
     * establish a new one.
     */
    if (ftpState->data.fd > 0) {
	comm_close(ftpState->data.fd);
	ftpState->data.fd = -1;
    }
    /*
     * Set up a listen socket on the same local address as the
     * control connection.
     */
    addr_len = sizeof(addr);
    if (getsockname(ftpState->ctrl.fd, (struct sockaddr *) &addr, &addr_len)) {
	debug(9, 0) ("ftpOpenListenSocket: getsockname(%d,..): %s\n",
	    ftpState->ctrl.fd, xstrerror());
	return -1;
    }
    /*
     * REUSEADDR is needed in fallback mode, since the same port is
     * used for both control and data.
     */
    if (fallback) {
	setsockopt(ftpState->ctrl.fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
	port = ntohs(addr.sin_port);
    }
    fd = comm_open(SOCK_STREAM,
	0,
	addr.sin_addr,
	port,
	COMM_NONBLOCKING | (fallback ? COMM_REUSEADDR : 0),
	storeUrl(ftpState->entry));
    debug(9, 3) ("ftpOpenListenSocket: Unconnected data socket created on FD %d\n", fd);
    if (fd < 0) {
	debug(9, 0) ("ftpOpenListenSocket: comm_open failed\n");
	return -1;
    }
    if (comm_listen(fd) < 0) {
	comm_close(fd);
	return -1;
    }
    ftpState->data.fd = fd;
    ftpState->data.port = comm_local_port(fd);
    ftpState->data.host = NULL;
    return fd;
}

static void
ftpSendPort(FtpStateData * ftpState)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    unsigned char *addrptr;
    unsigned char *portptr;
    debug(9, 3) ("This is ftpSendPort\n");
    ftpState->flags.pasv_supported = 0;
    fd = ftpOpenListenSocket(ftpState, 0);
    addr_len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *) &addr, &addr_len)) {
	debug(9, 0) ("ftpSendPort: getsockname(%d,..): %s\n", fd, xstrerror());
	/* XXX Need to set error message */
	ftpFail(ftpState);
	return;
    }
    addrptr = (unsigned char *) &addr.sin_addr.s_addr;
    portptr = (unsigned char *) &addr.sin_port;
    snprintf(cbuf, 1024, "PORT %d,%d,%d,%d,%d,%d\r\n",
	addrptr[0], addrptr[1], addrptr[2], addrptr[3],
	portptr[0], portptr[1]);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_PORT;
}

static void
ftpReadPort(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadPort\n");
    if (code != 200) {
	/* Fall back on using the same port as the control connection */
	debug(9, 3) ("PORT not supported by remote end\n");
	ftpOpenListenSocket(ftpState, 1);
    }
    ftpRestOrList(ftpState);
}

/* "read" handler to accept data connection */
static void
ftpAcceptDataConnection(int fd, void *data)
{
    FtpStateData *ftpState = data;
    struct sockaddr_in my_peer, me;
    debug(9, 3) ("ftpAcceptDataConnection\n");

    if (EBIT_TEST(ftpState->entry->flags, ENTRY_ABORTED)) {
	comm_close(ftpState->ctrl.fd);
	return;
    }
    fd = comm_accept(fd, &my_peer, &me);
    if (Config.Ftp.sanitycheck) {
	char *ipaddr = inet_ntoa(my_peer.sin_addr);
	if (strcmp(fd_table[ftpState->ctrl.fd].ipaddr, ipaddr) != 0) {
	    debug(9, 1) ("FTP data connection from unexpected server (%s:%d), expecting %s\n", ipaddr, (int) ntohs(my_peer.sin_port), fd_table[ftpState->ctrl.fd].ipaddr);
	    comm_close(fd);
	    commSetSelect(ftpState->data.fd,
		COMM_SELECT_READ,
		ftpAcceptDataConnection,
		ftpState,
		0);
	    return;
	}
    }
    if (fd < 0) {
	debug(9, 1) ("ftpHandleDataAccept: comm_accept(%d): %s\n", fd, xstrerror());
	/* XXX Need to set error message */
	ftpFail(ftpState);
	return;
    }
    /* Replace the Listen socket with the accepted data socket */
    comm_close(ftpState->data.fd);
    debug(9, 3) ("ftpAcceptDataConnection: Connected data socket on FD %d\n", fd);
    ftpState->data.fd = fd;
    ftpState->data.port = ntohs(my_peer.sin_port);
    ftpState->data.host = xstrdup(inet_ntoa(my_peer.sin_addr));
    commSetTimeout(ftpState->ctrl.fd, -1, NULL, NULL);
    commSetTimeout(ftpState->data.fd, Config.Timeout.read, ftpTimeout,
	ftpState);
    /* XXX We should have a flag to track connect state...
     *    host NULL -> not connected, port == local port
     *    host set  -> connected, port == remote port
     */
    /* Restart state (SENT_NLST/LIST/RETR) */
    FTP_SM_FUNCS[ftpState->state] (ftpState);
}

static void
ftpRestOrList(FtpStateData * ftpState)
{
    debug(9, 3) ("This is ftpRestOrList\n");
    if (ftpState->typecode == 'D') {
	ftpState->flags.isdir = 1;
	if (ftpState->flags.put) {
	    ftpSendMkdir(ftpState);	/* PUT name;type=d */
	} else {
	    ftpSendNlst(ftpState);	/* GET name;type=d  sec 3.2.2 of RFC 1738 */
	}
    } else if (ftpState->flags.put) {
	debug(9, 3) ("ftpRestOrList: Sending STOR request...\n");
	ftpSendStor(ftpState);
    } else if (ftpState->flags.isdir)
	ftpSendList(ftpState);
    else if (ftpRestartable(ftpState))
	ftpSendRest(ftpState);
    else
	ftpSendRetr(ftpState);
}

static void
ftpSendStor(FtpStateData * ftpState)
{
    if (ftpState->filepath != NULL) {
	/* Plain file upload */
	snprintf(cbuf, 1024, "STOR %s\r\n", ftpState->filepath);
	ftpWriteCommand(cbuf, ftpState);
	ftpState->state = SENT_STOR;
    } else if (httpHeaderGetSize(&ftpState->request->header, HDR_CONTENT_LENGTH) > 0) {
	/* File upload without a filename. use STOU to generate one */
	snprintf(cbuf, 1024, "STOU\r\n");
	ftpWriteCommand(cbuf, ftpState);
	ftpState->state = SENT_STOR;
    } else {
	/* No file to transfer. Only create directories if needed */
	ftpSendReply(ftpState);
    }
}

static void
ftpReadStor(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadStor\n");
    if (code == 125 || (code == 150 && ftpState->data.host)) {
	/* Begin data transfer */
	debug(9, 3) ("ftpReadStor: starting data transfer\n");
	commSetSelect(ftpState->data.fd,
	    COMM_SELECT_WRITE,
	    ftpDataWrite,
	    ftpState,
	    Config.Timeout.read);
	/*
	 * Cancel the timeout on the Control socket and
	 * establish one on the data socket.
	 */
	commSetTimeout(ftpState->ctrl.fd, -1, NULL, NULL);
	commSetTimeout(ftpState->data.fd, Config.Timeout.read, ftpTimeout,
	    ftpState);
	ftpState->state = WRITING_DATA;
	debug(9, 3) ("ftpReadStor: writing data channel\n");
    } else if (code == 150) {
	/* Accept data channel */
	debug(9, 3) ("ftpReadStor: accepting data channel\n");
	commSetSelect(ftpState->data.fd,
	    COMM_SELECT_READ,
	    ftpAcceptDataConnection,
	    ftpState,
	    0);
    } else {
	debug(9, 3) ("ftpReadStor: Unexpected reply code %03d\n", code);
	ftpFail(ftpState);
    }
}

static void
ftpSendRest(FtpStateData * ftpState)
{
    snprintf(cbuf, 1024, "REST %" PRINTF_OFF_T "\r\n", ftpState->restart_offset);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_REST;
}

static int
ftpRestartable(FtpStateData * ftpState)
{
    if (ftpState->restart_offset > 0)
	return 1;
    if (!ftpState->request->range)
	return 0;
    if (!ftpState->flags.binary)
	return 0;
    if (ftpState->size <= 0)
	return 0;

    ftpState->restart_offset = httpHdrRangeLowestOffset(ftpState->request->range, (size_t) ftpState->size);
    if (ftpState->restart_offset <= 0)
	return 0;
    return 1;
}

static void
ftpReadRest(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadRest\n");
    assert(ftpState->restart_offset > 0);
    if (code == 350) {
	ftpState->restarted_offset = ftpState->restart_offset;
	ftpSendRetr(ftpState);
    } else if (code > 0) {
	debug(9, 3) ("ftpReadRest: REST not supported\n");
	ftpState->flags.rest_supported = 0;
	ftpSendRetr(ftpState);
    } else {
	ftpFail(ftpState);
    }
}

static void
ftpSendList(FtpStateData * ftpState)
{
    if (ftpState->filepath) {
	snprintf(cbuf, 1024, "LIST %s\r\n", ftpState->filepath);
    } else {
	snprintf(cbuf, 1024, "LIST\r\n");
    }
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_LIST;
}

static void
ftpSendNlst(FtpStateData * ftpState)
{
    ftpState->flags.tried_nlst = 1;
    if (ftpState->filepath) {
	snprintf(cbuf, 1024, "NLST %s\r\n", ftpState->filepath);
    } else {
	snprintf(cbuf, 1024, "NLST\r\n");
    }
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_NLST;
}

static void
ftpReadList(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadList\n");
    if (code == 125 || (code == 150 && ftpState->data.host)) {
	/* Begin data transfer */
	commSetSelect(ftpState->data.fd,
	    COMM_SELECT_READ,
	    ftpDataRead,
	    ftpState,
	    Config.Timeout.read);
	commSetDefer(ftpState->data.fd, fwdCheckDeferRead, ftpState->entry);
	ftpState->state = READING_DATA;
	/*
	 * Cancel the timeout on the Control socket and establish one
	 * on the data socket
	 */
	commSetTimeout(ftpState->ctrl.fd, -1, NULL, NULL);
	commSetTimeout(ftpState->data.fd, Config.Timeout.read, ftpTimeout, ftpState);
	return;
    } else if (code == 150) {
	/* Accept data channel */
	commSetSelect(ftpState->data.fd,
	    COMM_SELECT_READ,
	    ftpAcceptDataConnection,
	    ftpState,
	    0);
	/*
	 * Cancel the timeout on the Control socket and establish one
	 * on the data socket
	 */
	commSetTimeout(ftpState->ctrl.fd, -1, NULL, NULL);
	commSetTimeout(ftpState->data.fd, Config.Timeout.read, ftpTimeout, ftpState);
	return;
    } else if (!ftpState->flags.tried_nlst && code > 300) {
	ftpSendNlst(ftpState);
    } else {
	ftpFail(ftpState);
	return;
    }
}

static void
ftpSendRetr(FtpStateData * ftpState)
{
    assert(ftpState->filepath != NULL);
    snprintf(cbuf, 1024, "RETR %s\r\n", ftpState->filepath);
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_RETR;
}

static void
ftpReadRetr(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadRetr\n");
    if (code == 125 || (code == 150 && ftpState->data.host)) {
	/* Begin data transfer */
	debug(9, 3) ("ftpReadRetr: reading data channel\n");
	commSetSelect(ftpState->data.fd,
	    COMM_SELECT_READ,
	    ftpDataRead,
	    ftpState,
	    Config.Timeout.read);
	commSetDefer(ftpState->data.fd, fwdCheckDeferRead, ftpState->entry);
	ftpState->state = READING_DATA;
	/*
	 * Cancel the timeout on the Control socket and establish one
	 * on the data socket
	 */
	commSetTimeout(ftpState->ctrl.fd, -1, NULL, NULL);
	commSetTimeout(ftpState->data.fd, Config.Timeout.read, ftpTimeout,
	    ftpState);
    } else if (code == 150) {
	/* Accept data channel */
	commSetSelect(ftpState->data.fd,
	    COMM_SELECT_READ,
	    ftpAcceptDataConnection,
	    ftpState,
	    0);
	/*
	 * Cancel the timeout on the Control socket and establish one
	 * on the data socket
	 */
	commSetTimeout(ftpState->ctrl.fd, -1, NULL, NULL);
	commSetTimeout(ftpState->data.fd, Config.Timeout.read, ftpTimeout,
	    ftpState);
    } else if (code >= 300) {
	if (!ftpState->flags.try_slash_hack) {
	    /* Try this as a directory missing trailing slash... */
	    ftpHackShortcut(ftpState, ftpSendCwd);
	} else {
	    ftpFail(ftpState);
	}
    } else {
	ftpFail(ftpState);
    }
}

static void
ftpReadTransferDone(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpReadTransferDone\n");
    if (code == 226) {
	/* Connection closed; retrieval done. */
	if (ftpState->flags.html_header_sent)
	    ftpListingFinish(ftpState);
	fwdUnregister(ftpState->ctrl.fd, ftpState->fwd);
	fwdComplete(ftpState->fwd);
	ftpSendQuit(ftpState);
    } else {			/* != 226 */
	debug(9, 1) ("ftpReadTransferDone: Got code %d after reading data\n",
	    code);
	ftpFailed(ftpState, ERR_FTP_FAILURE);
	/* ftpFailed closes ctrl.fd and frees ftpState */
	return;
    }
}

/* This will be called when there is data available to put */
static void
ftpRequestBody(char *buf, ssize_t size, void *data)
{
    FtpStateData *ftpState = (FtpStateData *) data;
    debug(9, 3) ("ftpRequestBody: buf=%p size=%d ftpState=%p\n", buf, (int) size, data);
    ftpState->data.offset = size;
    if (size > 0) {
	/* DataWrite */
	comm_write(ftpState->data.fd, buf, size, ftpDataWriteCallback, data, NULL);
    } else if (size < 0) {
	/* Error */
	debug(9, 1) ("ftpRequestBody: request aborted\n");
	ftpFailed(ftpState, ERR_READ_ERROR);
    } else if (size == 0) {
	/* End of transfer */
	ftpDataComplete(ftpState);
    }
}

/* This will be called when the put write is completed */
static void
ftpDataWriteCallback(int fd, char *buf, size_t size, int err, void *data)
{
    FtpStateData *ftpState = (FtpStateData *) data;
    if (err == COMM_ERR_CLOSING)
	return;
    if (!err) {
	/* Shedule the rest of the request */
	requestReadBody(ftpState->request, ftpState->data.buf, ftpState->data.size, ftpRequestBody, ftpState);
    } else {
	debug(9, 1) ("ftpDataWriteCallback: write error: %s\n", xstrerror());
	ftpFailed(ftpState, ERR_WRITE_ERROR);
    }
}

static void
ftpDataWrite(int ftp, void *data)
{
    FtpStateData *ftpState = (FtpStateData *) data;
    debug(9, 3) ("ftpDataWrite\n");
    /* This starts the body transfer */
    requestReadBody(ftpState->request, ftpState->data.buf, ftpState->data.size, ftpRequestBody, ftpState);
}

static void
ftpWriteTransferDone(FtpStateData * ftpState)
{
    int code = ftpState->ctrl.replycode;
    debug(9, 3) ("This is ftpWriteTransferDone\n");
    if (code != 226) {
	debug(9, 1) ("ftpReadTransferDone: Got code %d after sending data\n",
	    code);
	ftpFailed(ftpState, ERR_FTP_PUT_ERROR);
	return;
    }
    storeTimestampsSet(ftpState->entry);	/* XXX Is this needed? */
    ftpSendReply(ftpState);
}

static void
ftpSendQuit(FtpStateData * ftpState)
{
    assert(ftpState->ctrl.fd > -1);
    snprintf(cbuf, 1024, "QUIT\r\n");
    ftpWriteCommand(cbuf, ftpState);
    ftpState->state = SENT_QUIT;
}

static void
ftpReadQuit(FtpStateData * ftpState)
{
    comm_close(ftpState->ctrl.fd);
}

static void
ftpTrySlashHack(FtpStateData * ftpState)
{
    char *path;
    ftpState->flags.try_slash_hack = 1;
    /* Free old paths */
    if (ftpState->pathcomps)
	wordlistDestroy(&ftpState->pathcomps);
    safe_free(ftpState->filepath);
    /* Build the new path (urlpath begins with /) */
    path = xstrdup(strBuf(ftpState->request->urlpath));
    rfc1738_unescape(path);
    ftpState->filepath = path;
    /* And off we go */
    ftpGetFile(ftpState);
}

/* Forget hack status. Next error is shown to the user */
static void
ftpUnhack(FtpStateData * ftpState)
{
    if (ftpState->old_request != NULL) {
	safe_free(ftpState->old_request);
	safe_free(ftpState->old_reply);
    }
}

static void
ftpHackShortcut(FtpStateData * ftpState, FTPSM * nextState)
{
    /* Clear some unwanted state */
    ftpState->restarted_offset = 0;
    ftpState->restart_offset = 0;
    /* Save old error message & some state info */
    if (ftpState->old_request == NULL) {
	ftpState->old_request = ftpState->ctrl.last_command;
	ftpState->ctrl.last_command = NULL;
	ftpState->old_reply = ftpState->ctrl.last_reply;
	ftpState->ctrl.last_reply = NULL;
	if (ftpState->pathcomps == NULL && ftpState->filepath != NULL)
	    ftpState->old_filepath = xstrdup(ftpState->filepath);
    }
    /* Jump to the "hack" state */
    nextState(ftpState);
}

static void
ftpFail(FtpStateData * ftpState)
{
    debug(9, 3) ("ftpFail\n");
    /* Try the / hack to support "Netscape" FTP URL's for retreiving files */
    if (!ftpState->flags.isdir &&	/* Not a directory */
	!ftpState->flags.try_slash_hack &&	/* Not in slash hack */
	ftpState->mdtm <= 0 && ftpState->size < 0 &&	/* Not known as a file */
	strNCaseCmp(ftpState->request->urlpath, "/%2f", 4) != 0) {	/* No slash encoded */
	switch (ftpState->state) {
	case SENT_CWD:
	case SENT_RETR:
	    /* Try the / hack */
	    ftpHackShortcut(ftpState, ftpTrySlashHack);
	    return;
	default:
	    break;
	}
    }
    ftpFailed(ftpState, ERR_NONE);
    /* ftpFailed closes ctrl.fd and frees ftpState */
}

static void
ftpFailed(FtpStateData * ftpState, err_type error)
{
    StoreEntry *entry = ftpState->entry;
    if (entry->mem_obj->inmem_hi == 0)
	ftpFailedErrorMessage(ftpState, error);
    if (ftpState->data.fd > -1) {
	comm_close(ftpState->data.fd);
	ftpState->data.fd = -1;
    }
    comm_close(ftpState->ctrl.fd);
}

static void
ftpFailedErrorMessage(FtpStateData * ftpState, err_type error)
{
    ErrorState *err;
    const char *command, *reply;
    /* Translate FTP errors into HTTP errors */
    err = NULL;
    switch (error) {
    case ERR_NONE:
	switch (ftpState->state) {
	case SENT_USER:
	case SENT_PASS:
	    if (ftpState->ctrl.replycode > 500)
		if (ftpState->password_url)
		    err = errorCon(ERR_FTP_FORBIDDEN, HTTP_FORBIDDEN);
		else
		    err = errorCon(ERR_FTP_FORBIDDEN, HTTP_UNAUTHORIZED);
	    else if (ftpState->ctrl.replycode == 421)
		err = errorCon(ERR_FTP_UNAVAILABLE, HTTP_SERVICE_UNAVAILABLE);
	    break;
	case SENT_CWD:
	case SENT_RETR:
	    if (ftpState->ctrl.replycode == 550)
		err = errorCon(ERR_FTP_NOT_FOUND, HTTP_NOT_FOUND);
	    break;
	default:
	    break;
	}
	break;
    case ERR_READ_TIMEOUT:
	err = errorCon(error, HTTP_GATEWAY_TIMEOUT);
	break;
    default:
	err = errorCon(error, HTTP_BAD_GATEWAY);
	break;
    }
    if (err == NULL)
	err = errorCon(ERR_FTP_FAILURE, HTTP_BAD_GATEWAY);
    err->xerrno = errno;
    err->request = requestLink(ftpState->request);
    err->ftp.server_msg = ftpState->ctrl.message;
    ftpState->ctrl.message = NULL;
    if (ftpState->old_request)
	command = ftpState->old_request;
    else
	command = ftpState->ctrl.last_command;
    if (command && strncmp(command, "PASS", 4) == 0)
	command = "PASS <yourpassword>";
    if (ftpState->old_reply)
	reply = ftpState->old_reply;
    else
	reply = ftpState->ctrl.last_reply;
    if (command)
	err->ftp.request = xstrdup(command);
    if (reply)
	err->ftp.reply = xstrdup(reply);
    fwdFail(ftpState->fwd, err);
}

static void
ftpSendReply(FtpStateData * ftpState)
{
    ErrorState *err;
    int code = ftpState->ctrl.replycode;
    http_status http_code;
    err_type err_code = ERR_NONE;
    debug(9, 5) ("ftpSendReply: %s, code %d\n",
	storeUrl(ftpState->entry), code);
    if (cbdataValid(ftpState))
	debug(9, 5) ("ftpSendReply: ftpState (%p) is valid!\n", ftpState);
    if (code == 226) {
	err_code = (ftpState->mdtm > 0) ? ERR_FTP_PUT_MODIFIED : ERR_FTP_PUT_CREATED;
	http_code = (ftpState->mdtm > 0) ? HTTP_ACCEPTED : HTTP_CREATED;
    } else if (code == 227) {
	err_code = ERR_FTP_PUT_CREATED;
	http_code = HTTP_CREATED;
    } else {
	err_code = ERR_FTP_PUT_ERROR;
	http_code = HTTP_INTERNAL_SERVER_ERROR;
    }
    err = errorCon(err_code, http_code);
    err->request = requestLink(ftpState->request);
    if (ftpState->old_request)
	err->ftp.request = xstrdup(ftpState->old_request);
    else
	err->ftp.request = xstrdup(ftpState->ctrl.last_command);
    if (ftpState->old_reply)
	err->ftp.reply = xstrdup(ftpState->old_reply);
    else if (ftpState->ctrl.last_reply)
	err->ftp.reply = xstrdup(ftpState->ctrl.last_reply);
    else
	err->ftp.reply = xstrdup("");
    errorAppendEntry(ftpState->entry, err);
    storeBufferFlush(ftpState->entry);
    ftpSendQuit(ftpState);
}

static void
ftpAppendSuccessHeader(FtpStateData * ftpState)
{
    const char *mime_type = NULL;
    const char *mime_enc = NULL;
    String urlpath = ftpState->request->urlpath;
    const char *filename = NULL;
    const char *t = NULL;
    StoreEntry *e = ftpState->entry;
    http_reply *reply = e->mem_obj->reply;
    http_version_t version;

    if (ftpState->flags.http_header_sent)
	return;
    ftpState->flags.http_header_sent = 1;
    assert(e->mem_obj->inmem_hi == 0);
    EBIT_CLR(e->flags, ENTRY_FWD_HDR_WAIT);
    filename = (t = strRChr(urlpath, '/')) ? t + 1 : strBuf(urlpath);
    if (ftpState->flags.isdir) {
	mime_type = "text/html";
    } else {
	switch (ftpState->typecode) {
	case 'I':
	    mime_type = "application/octet-stream";
	    mime_enc = mimeGetContentEncoding(filename);
	    break;
	case 'A':
	    mime_type = "text/plain";
	    break;
	default:
	    mime_type = mimeGetContentType(filename);
	    mime_enc = mimeGetContentEncoding(filename);
	    break;
	}
    }
    storeBuffer(e);
    httpReplyReset(reply);
    /* set standard stuff */
    if (ftpState->restarted_offset) {
	/* Partial reply */
	HttpHdrRangeSpec range_spec;
	range_spec.offset = ftpState->restarted_offset;
	range_spec.length = ftpState->size - ftpState->restarted_offset;
	httpBuildVersion(&version, 1, 0);
	httpReplySetHeaders(reply, version, HTTP_PARTIAL_CONTENT, "Gatewaying",
	    mime_type, ftpState->size - ftpState->restarted_offset, ftpState->mdtm, -2);
	httpHeaderAddContRange(&reply->header, range_spec, ftpState->size);
    } else {
	/* Full reply */
	httpBuildVersion(&version, 1, 0);
	httpReplySetHeaders(reply, version, HTTP_OK, "Gatewaying",
	    mime_type, ftpState->size, ftpState->mdtm, -2);
    }
    /* additional info */
    if (mime_enc)
	httpHeaderPutStr(&reply->header, HDR_CONTENT_ENCODING, mime_enc);
    httpReplySwapOut(reply, e);
    storeBufferFlush(e);
    reply->hdr_sz = e->mem_obj->inmem_hi;
    storeTimestampsSet(e);
    if (ftpState->flags.authenticated) {
	/*
	 * Authenticated requests can't be cached.
	 */
	storeRelease(e);
    } else if (EBIT_TEST(e->flags, ENTRY_CACHABLE) && !ftpState->restarted_offset) {
	storeSetPublicKey(e);
    } else {
	storeRelease(e);
    }
}

static void
ftpAuthRequired(HttpReply * old_reply, request_t * request, const char *realm)
{
    ErrorState *err = errorCon(ERR_CACHE_ACCESS_DENIED, HTTP_UNAUTHORIZED);
    HttpReply *rep;
    err->request = requestLink(request);
    rep = errorBuildReply(err);
    errorStateFree(err);
    /* add Authenticate header */
    httpHeaderPutAuth(&rep->header, "Basic", realm);
    /* move new reply to the old one */
    httpReplyAbsorb(old_reply, rep);
}

char *
ftpUrlWith2f(const request_t * request)
{
    LOCAL_ARRAY(char, buf, MAX_URL);
    LOCAL_ARRAY(char, loginbuf, MAX_LOGIN_SZ + 1);
    LOCAL_ARRAY(char, portbuf, 32);
    char *t;
    portbuf[0] = '\0';
    if (request->protocol != PROTO_FTP)
	return NULL;
    if (request->port != urlDefaultPort(request->protocol))
	snprintf(portbuf, 32, ":%d", request->port);
    loginbuf[0] = '\0';
    if ((int) strlen(request->login) > 0) {
	xstrncpy(loginbuf, request->login, sizeof(loginbuf) - 2);
	if ((t = strchr(loginbuf, ':')))
	    *t = '\0';
	strcat(loginbuf, "@");
    }
    snprintf(buf, MAX_URL, "%s://%s%s%s%s%s",
	ProtocolStr[request->protocol],
	loginbuf,
	request->host,
	portbuf,
	"/%2f",
	strBuf(request->urlpath));
    if ((t = strchr(buf, '?')))
	*t = '\0';
    return buf;
}
