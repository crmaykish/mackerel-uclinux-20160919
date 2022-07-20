
/*
 * $Id: main.c,v 1.345.2.25 2005/04/20 21:52:26 hno Exp $
 *
 * DEBUG: section 1     Startup and Main Loop
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

/* for error reporting from xmalloc and friends */
extern void (*failure_notify) (const char *);

static int opt_no_daemon = 0;
static int opt_parse_cfg_only = 0;
static char *opt_syslog_facility = NULL;
static int httpPortNumOverride = 1;
static int icpPortNumOverride = 1;	/* Want to detect "-u 0" */
static int configured_once = 0;
#if MALLOC_DBG
static int malloc_debug_level = 0;
#endif
static volatile int do_reconfigure = 0;
static volatile int do_rotate = 0;
static volatile int do_shutdown = 0;

static void mainRotate(void);
static void mainReconfigure(void);
static SIGHDLR rotate_logs;
static SIGHDLR reconfigure;
#if ALARM_UPDATES_TIME
static SIGHDLR time_tick;
#endif
static void mainInitialize(void);
static void usage(void);
static void mainParseOptions(int, char **);
static void sendSignal(void);
static void serverConnectionsOpen(void);
static void watch_child(char **);
static void setEffectiveUser(void);
#if MEM_GEN_TRACE
extern void log_trace_done();
extern void log_trace_init(char *);
#endif
static EVH SquidShutdown;
static void mainSetCwd(void);
static int checkRunningPid(void);

static const char *squid_start_script = "squid_start";

#if TEST_ACCESS
#include "test_access.c"
#endif

static void
usage(void)
{
    fprintf(stderr,
	"Usage: %s [-dhvzCDFNRVYX] [-s | -l facility] [-f config-file] [-[au] port] [-k signal]\n"
	"       -a port   Specify HTTP port number (default: %d).\n"
	"       -d level  Write debugging to stderr also.\n"
	"       -f file   Use given config-file instead of\n"
	"                 %s\n"
	"       -h        Print help message.\n"
	"       -k reconfigure|rotate|shutdown|interrupt|kill|debug|check|parse\n"
	"                 Parse configuration file, then send signal to \n"
	"                 running copy (except -k parse) and exit.\n"
	"       -s | -l facility\n"
	"                 Enable logging to syslog.\n"
	"       -u port   Specify ICP port number (default: %d), disable with 0.\n"
	"       -v        Print version.\n"
	"       -z        Create swap directories\n"
	"       -C        Do not catch fatal signals.\n"
	"       -D        Disable initial DNS tests.\n"
	"       -F        Don't serve any requests until store is rebuilt.\n"
	"       -N        No daemon mode.\n"
	"       -R        Do not set REUSEADDR on port.\n"
	"       -S        Double-check swap during rebuild.\n"
	"       -V        Virtual host httpd-accelerator.\n"
	"       -X        Force full debugging.\n"
	"       -Y        Only return UDP_HIT or UDP_MISS_NOFETCH during fast reload.\n",
	appname, CACHE_HTTP_PORT, DefaultConfigFile, CACHE_ICP_PORT);
    exit(1);
}

static void
mainParseOptions(int argc, char *argv[])
{
    extern char *optarg;
    int c;

    while ((c = getopt(argc, argv, "CDFNRSVYXa:d:f:hk:m::sl:u:vz?")) != -1) {
	switch (c) {
	case 'C':
	    opt_catch_signals = 0;
	    break;
	case 'D':
	    opt_dns_tests = 0;
	    break;
	case 'F':
	    opt_foreground_rebuild = 1;
	    break;
	case 'N':
	    opt_no_daemon = 1;
	    break;
	case 'R':
	    opt_reuseaddr = 0;
	    break;
	case 'S':
	    opt_store_doublecheck = 1;
	    break;
	case 'V':
	    vhost_mode = 1;
	    break;
	case 'X':
	    /* force full debugging */
	    sigusr2_handle(SIGUSR2);
	    break;
	case 'Y':
	    opt_reload_hit_only = 1;
	    break;
	case 'a':
	    httpPortNumOverride = atoi(optarg);
	    break;
	case 'd':
	    opt_debug_stderr = atoi(optarg);
	    break;
	case 'f':
	    xfree(ConfigFile);
	    ConfigFile = xstrdup(optarg);
	    break;
	case 'h':
	    usage();
	    break;
	case 'k':
	    if ((int) strlen(optarg) < 1)
		usage();
	    if (!strncmp(optarg, "reconfigure", strlen(optarg)))
		opt_send_signal = SIGHUP;
	    else if (!strncmp(optarg, "rotate", strlen(optarg)))
#ifdef _SQUID_LINUX_THREADS_
		opt_send_signal = SIGQUIT;
#else
		opt_send_signal = SIGUSR1;
#endif
	    else if (!strncmp(optarg, "debug", strlen(optarg)))
#ifdef _SQUID_LINUX_THREADS_
		opt_send_signal = SIGTRAP;
#else
		opt_send_signal = SIGUSR2;
#endif
	    else if (!strncmp(optarg, "shutdown", strlen(optarg)))
		opt_send_signal = SIGTERM;
	    else if (!strncmp(optarg, "interrupt", strlen(optarg)))
		opt_send_signal = SIGINT;
	    else if (!strncmp(optarg, "kill", strlen(optarg)))
		opt_send_signal = SIGKILL;
	    else if (!strncmp(optarg, "check", strlen(optarg)))
		opt_send_signal = 0;	/* SIGNULL */
	    else if (!strncmp(optarg, "parse", strlen(optarg)))
		opt_parse_cfg_only = 1;		/* parse cfg file only */
	    else
		usage();
	    break;
	case 'm':
	    if (optarg) {
#if MALLOC_DBG
		malloc_debug_level = atoi(optarg);
		/* NOTREACHED */
		break;
#else
		fatal("Need to add -DMALLOC_DBG when compiling to use -mX option");
		/* NOTREACHED */
#endif
	    } else {
#if XMALLOC_TRACE
		xmalloc_trace = !xmalloc_trace;
#else
		fatal("Need to configure --enable-xmalloc-debug-trace to use -m option");
#endif
	    }
	case 'l':
	    opt_syslog_facility = xstrdup(optarg);
	case 's':
#if HAVE_SYSLOG
	    _db_set_syslog(opt_syslog_facility);
	    break;
#else
	    fatal("Logging to syslog not available on this platform");
	    /* NOTREACHED */
#endif
	case 'u':
	    icpPortNumOverride = atoi(optarg);
	    if (icpPortNumOverride < 0)
		icpPortNumOverride = 0;
	    break;
	case 'v':
	    printf("Squid Cache: Version %s\nconfigure options: %s\n", version_string, SQUID_CONFIGURE_OPTIONS);
	    exit(0);
	    /* NOTREACHED */
	case 'z':
	    opt_create_swap_dirs = 1;
	    break;
	case '?':
	default:
	    usage();
	    break;
	}
    }
}

/* ARGSUSED */
static void
rotate_logs(int sig)
{
    do_rotate = 1;
#if !HAVE_SIGACTION
    signal(sig, rotate_logs);
#endif
}

#if ALARM_UPDATES_TIME
static void
time_tick(int sig)
{
    getCurrentTime();
    alarm(1);
#if !HAVE_SIGACTION
    signal(sig, time_tick);
#endif
}

#endif

/* ARGSUSED */
static void
reconfigure(int sig)
{
    do_reconfigure = 1;
#if !HAVE_SIGACTION
    signal(sig, reconfigure);
#endif
}

void
shut_down(int sig)
{
    do_shutdown = sig == SIGINT ? -1 : 1;
#ifdef KILL_PARENT_OPT
    if (getppid() > 1) {
	debug(1, 1) ("Killing RunCache, pid %ld\n", (long) getppid());
	if (kill(getppid(), sig) < 0)
	    debug(1, 1) ("kill %ld: %s\n", (long) getppid(), xstrerror());
    }
#endif
#if SA_RESETHAND == 0
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
#endif
}

static void
serverConnectionsOpen(void)
{
    clientOpenListenSockets();
    icpConnectionsOpen();
#if USE_HTCP
    htcpInit();
#endif
#ifdef SQUID_SNMP
    snmpConnectionOpen();
#endif
#if USE_WCCP
    wccpConnectionOpen();
#endif
    clientdbInit();
    icmpOpen();
    netdbInit();
    asnInit();
    peerSelectInit();
#if USE_CARP
    carpInit();
#endif
}

void
serverConnectionsClose(void)
{
    assert(shutting_down || reconfiguring);
    clientHttpConnectionsClose();
    icpConnectionShutdown();
#if USE_HTCP
    htcpSocketShutdown();
#endif
    icmpClose();
#ifdef SQUID_SNMP
    snmpConnectionShutdown();
#endif
#if USE_WCCP
    wccpConnectionShutdown();
#endif
    asnFreeMemory();
}

static void
mainReconfigure(void)
{
    debug(1, 1) ("Reconfiguring Squid Cache (version %s)...\n", version_string);
    reconfiguring = 1;
    /* Already called serverConnectionsClose and ipcacheShutdownServers() */
    serverConnectionsClose();
    icpConnectionClose();
#if USE_HTCP
    htcpSocketClose();
#endif
#ifdef SQUID_SNMP
    snmpConnectionClose();
#endif
#if USE_WCCP
    wccpConnectionClose();
#endif
#if USE_DNSSERVERS
    dnsShutdown();
#else
    idnsShutdown();
#endif
#ifdef HS_FEAT_ICAP
    icapClose();
#endif
    redirectShutdown();
    authenticateShutdown();
    externalAclShutdown();
    storeDirCloseSwapLogs();
    storeLogClose();
    accessLogClose();
    useragentLogClose();
    refererCloseLog();
    errorClean();
    enter_suid();		/* root to read config file */
    parseConfigFile(ConfigFile);
    setEffectiveUser();
    _db_init(Config.Log.log, Config.debugOptions);
    ipcache_restart();		/* clear stuck entries */
    authenticateUserCacheRestart();	/* clear stuck ACL entries */
    fqdncache_restart();	/* sigh, fqdncache too */
    parseEtcHosts();
    errorInitialize();		/* reload error pages */
    accessLogInit();
    storeLogOpen();
    useragentOpenLog();
    refererOpenLog();
#if USE_DNSSERVERS
    dnsInit();
#else
    idnsInit();
#endif
    redirectInit();
#ifdef HS_FEAT_ICAP
    icapInit();
#endif
    authenticateInit(&Config.authConfig);
    externalAclInit();
#if USE_WCCP
    wccpInit();
#endif
    serverConnectionsOpen();
    if (theOutIcpConnection >= 0) {
	if (!Config2.Accel.on || Config.onoff.accel_with_proxy)
	    neighbors_open(theOutIcpConnection);
	else
	    debug(1, 1) ("ICP port disabled in httpd_accelerator mode\n");
    }
    storeDirOpenSwapLogs();
    mimeInit(Config.mimeTablePathname);
    eventCleanup();
    writePidFile();		/* write PID file */
    debug(1, 1) ("Ready to serve requests.\n");
    reconfiguring = 0;
}

static void
mainRotate(void)
{
    icmpClose();
#if USE_DNSSERVERS
    dnsShutdown();
#endif
    redirectShutdown();
    authenticateShutdown();
    externalAclShutdown();
    _db_rotate_log();		/* cache.log */
    storeDirWriteCleanLogs(1);
    storeLogRotate();		/* store.log */
    accessLogRotate();		/* access.log */
    useragentRotateLog();	/* useragent.log */
    refererRotateLog();		/* referer.log */
#if WIP_FWD_LOG
    fwdLogRotate();
#endif
    icmpOpen();
#if USE_DNSSERVERS
    dnsInit();
#endif
    redirectInit();
    authenticateInit(&Config.authConfig);
    externalAclInit();
}

static void
setEffectiveUser(void)
{
    leave_suid();		/* Run as non privilegied user */
#ifdef _SQUID_OS2_
    return;
#endif
    if (geteuid() == 0) {
	debug(0, 0) ("Squid is not safe to run as root!  If you must\n");
	debug(0, 0) ("start Squid as root, then you must configure\n");
	debug(0, 0) ("it to run as a non-priveledged user with the\n");
	debug(0, 0) ("'cache_effective_user' option in the config file.\n");
	fatal("Don't run Squid as root, set 'cache_effective_user'!");
    }
}

static void
mainSetCwd(void)
{
    char pathbuf[MAXPATHLEN];
    if (Config.coredump_dir) {
	if (0 == strcmp("none", Config.coredump_dir)) {
	    (void) 0;
	} else if (chdir(Config.coredump_dir) == 0) {
	    debug(0, 1) ("Set Current Directory to %s\n", Config.coredump_dir);
	    return;
	} else {
	    debug(50, 0) ("chdir: %s: %s\n", Config.coredump_dir, xstrerror());
	}
    }
    /* If we don't have coredump_dir or couldn't cd there, report current dir */
    if (getcwd(pathbuf, MAXPATHLEN)) {
	debug(0, 1) ("Current Directory is %s\n", pathbuf);
    } else {
	debug(50, 0) ("WARNING: Can't find current directory, getcwd: %s\n", xstrerror());
    }
}

static void
mainInitialize(void)
{
    /* chroot if configured to run inside chroot */
    if (Config.chroot_dir && chroot(Config.chroot_dir)) {
	fatal("failed to chroot");
    }
    if (opt_catch_signals) {
	squid_signal(SIGSEGV, death, SA_NODEFER | SA_RESETHAND);
	squid_signal(SIGBUS, death, SA_NODEFER | SA_RESETHAND);
    }
    squid_signal(SIGPIPE, SIG_IGN, SA_RESTART);
    squid_signal(SIGCHLD, sig_child, SA_NODEFER | SA_RESTART);

    setEffectiveUser();
    assert(Config.Sockaddr.http);
    if (httpPortNumOverride != 1)
	Config.Sockaddr.http->s.sin_port = htons(httpPortNumOverride);
    if (icpPortNumOverride != 1)
	Config.Port.icp = (u_short) icpPortNumOverride;

    _db_init(Config.Log.log, Config.debugOptions);
    fd_open(fileno(debug_log), FD_LOG, Config.Log.log);
#if MEM_GEN_TRACE
    log_trace_init("/tmp/squid.alloc");
#endif
    debug(1, 0) ("Starting Squid Cache version %s for %s...\n",
	version_string,
	CONFIG_HOST_TYPE);
    debug(1, 1) ("Process ID %d\n", (int) getpid());
    debug(1, 1) ("With %d file descriptors available\n", Squid_MaxFD);

    if (!configured_once)
	disk_init();		/* disk_init must go before ipcache_init() */
    ipcache_init();
    fqdncache_init();
    parseEtcHosts();
#if USE_DNSSERVERS
    dnsInit();
#else
    idnsInit();
#endif
    redirectInit();
#ifdef HS_FEAT_ICAP
    icapInit();
#endif
    authenticateInit(&Config.authConfig);
    externalAclInit();
    useragentOpenLog();
    refererOpenLog();
    httpHeaderInitModule();	/* must go before any header processing (e.g. the one in errorInitialize) */
    httpReplyInitModule();	/* must go before accepting replies */
    errorInitialize();
    accessLogInit();
#if USE_IDENT
    identInit();
#endif
#ifdef SQUID_SNMP
    snmpInit();
#endif
#if MALLOC_DBG
    malloc_debug(0, malloc_debug_level);
#endif

    if (!configured_once) {
#if USE_UNLINKD
	unlinkdInit();
#endif
	urlInitialize();
	cachemgrInit();
	statInit();
	storeInit();
	mainSetCwd();
	/* after this point we want to see the mallinfo() output */
	do_mallinfo = 1;
	mimeInit(Config.mimeTablePathname);
	pconnInit();
	refreshInit();
#if DELAY_POOLS
	delayPoolsInit();
#endif
	fwdInit();
    }
#if USE_WCCP
    wccpInit();
#endif
    serverConnectionsOpen();
    if (theOutIcpConnection >= 0) {
	if (!Config2.Accel.on || Config.onoff.accel_with_proxy)
	    neighbors_open(theOutIcpConnection);
	else
	    debug(1, 1) ("ICP port disabled in httpd_accelerator mode\n");
    }
    if (Config.chroot_dir)
	no_suid();
    if (!configured_once)
	writePidFile();		/* write PID file */

#ifdef _SQUID_LINUX_THREADS_
    squid_signal(SIGQUIT, rotate_logs, SA_RESTART);
    squid_signal(SIGTRAP, sigusr2_handle, SA_RESTART);
#else
    squid_signal(SIGUSR1, rotate_logs, SA_RESTART);
    squid_signal(SIGUSR2, sigusr2_handle, SA_RESTART);
#endif
    squid_signal(SIGHUP, reconfigure, SA_RESTART);
    squid_signal(SIGTERM, shut_down, SA_NODEFER | SA_RESETHAND | SA_RESTART);
    squid_signal(SIGINT, shut_down, SA_NODEFER | SA_RESETHAND | SA_RESTART);
#if ALARM_UPDATES_TIME
    squid_signal(SIGALRM, time_tick, SA_RESTART);
    alarm(1);
#endif
    memCheckInit();
    debug(1, 1) ("Ready to serve requests.\n");
    if (!configured_once) {
	eventAdd("storeMaintain", storeMaintainSwapSpace, NULL, 1.0, 1);
	if (Config.onoff.announce)
	    eventAdd("start_announce", start_announce, NULL, 3600.0, 1);
	eventAdd("ipcache_purgelru", ipcache_purgelru, NULL, 10.0, 1);
	eventAdd("fqdncache_purgelru", fqdncache_purgelru, NULL, 15.0, 1);
    }
    configured_once = 1;
}

int
main(int argc, char **argv)
{
    int errcount = 0;
    int loop_delay;
    mode_t oldmask;
#if defined(_SQUID_MSWIN_) || defined(_SQUID_CYGWIN_)
    int WIN32_init_err;
#endif

#if HAVE_SBRK
    sbrk_start = sbrk(0);
#endif

    debug_log = stderr;
#ifndef HAVE_POLL
    if (FD_SETSIZE < Squid_MaxFD)
	Squid_MaxFD = FD_SETSIZE;
#endif

#if defined(_SQUID_MSWIN_) || defined(_SQUID_CYGWIN_)
    if ((WIN32_init_err = WIN32_Subsystem_Init()))
	return WIN32_init_err;
#endif

    /* call mallopt() before anything else */
#if HAVE_MALLOPT
#ifdef M_GRAIN
    /* Round up all sizes to a multiple of this */
    mallopt(M_GRAIN, 16);
#endif
#ifdef M_MXFAST
    /* biggest size that is considered a small block */
    mallopt(M_MXFAST, 256);
#endif
#ifdef M_NBLKS
    /* allocate this many small blocks at once */
    mallopt(M_NLBLKS, 32);
#endif
#endif /* HAVE_MALLOPT */

    /*
     * The plan here is to set the umask to 007 (deny others for
     * read,write,execute), but only if the umask is not already
     * set.  Unfortunately, there is no way to get the current
     * umask value without setting it.
     */
    oldmask = umask(S_IRWXO);
    if (oldmask)
	umask(oldmask);

    memset(&local_addr, '\0', sizeof(struct in_addr));
    safe_inet_addr(localhost, &local_addr);
    memset(&any_addr, '\0', sizeof(struct in_addr));
    safe_inet_addr("0.0.0.0", &any_addr);
    memset(&no_addr, '\0', sizeof(struct in_addr));
    safe_inet_addr("255.255.255.255", &no_addr);
    squid_srandom(time(NULL));

    getCurrentTime();
    squid_start = current_time;
    failure_notify = fatal_dump;

    mainParseOptions(argc, argv);

    /* parse configuration file
     * note: in "normal" case this used to be called from mainInitialize() */
    {
	int parse_err;
	if (!ConfigFile)
	    ConfigFile = xstrdup(DefaultConfigFile);
	assert(!configured_once);
#if USE_LEAKFINDER
	leakInit();
#endif
	memInit();
	cbdataInit();
	eventInit();		/* eventInit() is required for config parsing */
	storeFsInit();		/* required for config parsing */
	authenticateSchemeInit();	/* required for config parsing */
	parse_err = parseConfigFile(ConfigFile);

	if (opt_parse_cfg_only)
	    return parse_err;
    }
    if (-1 == opt_send_signal)
	if (checkRunningPid())
	    exit(1);

#if TEST_ACCESS
    comm_init();
    comm_select_init();
    mainInitialize();
    test_access();
    return 0;
#endif

    /* send signal to running copy and exit */
    if (opt_send_signal != -1) {
	/* chroot if configured to run inside chroot */
	if (Config.chroot_dir && chroot(Config.chroot_dir)) {
	    fatal("failed to chroot");
	}
	sendSignal();
	/* NOTREACHED */
    }
    if (opt_create_swap_dirs) {
	/* chroot if configured to run inside chroot */
	if (Config.chroot_dir && chroot(Config.chroot_dir)) {
	    fatal("failed to chroot");
	}
	setEffectiveUser();
	debug(0, 0) ("Creating Swap Directories\n");
	storeCreateSwapDirectories();
	return 0;
    }
    if (!opt_no_daemon)
	watch_child(argv);
    setMaxFD();

    /* init comm module */
    comm_init();
    comm_select_init();

    if (opt_no_daemon) {
	/* we have to init fdstat here. */
	fd_open(0, FD_LOG, "stdin");
	fd_open(1, FD_LOG, "stdout");
	fd_open(2, FD_LOG, "stderr");
    }
    mainInitialize();

    /* main loop */
    for (;;) {
	if (do_reconfigure) {
	    mainReconfigure();
	    do_reconfigure = 0;
	} else if (do_rotate) {
	    mainRotate();
	    do_rotate = 0;
	} else if (do_shutdown) {
	    time_t wait = do_shutdown > 0 ? (int) Config.shutdownLifetime : 0;
	    debug(1, 1) ("Preparing for shutdown after %d requests\n",
		statCounter.client_http.requests);
	    debug(1, 1) ("Waiting %d seconds for active connections to finish\n",
		(int) wait);
	    do_shutdown = 0;
	    shutting_down = 1;
	    serverConnectionsClose();
	    eventAdd("SquidShutdown", SquidShutdown, NULL, (double) (wait + 1), 1);
	}
	eventRun();
	if ((loop_delay = eventNextTime()) < 0)
	    loop_delay = 0;
#if HAVE_POLL
	switch (comm_poll(loop_delay)) {
#else
	switch (comm_select(loop_delay)) {
#endif
	case COMM_OK:
	    errcount = 0;	/* reset if successful */
	    break;
	case COMM_ERROR:
	    errcount++;
	    debug(1, 0) ("Select loop Error. Retry %d\n", errcount);
	    if (errcount == 10)
		fatal_dump("Select Loop failed!");
	    break;
	case COMM_TIMEOUT:
	    break;
	case COMM_SHUTDOWN:
	    SquidShutdown(NULL);
	    break;
	default:
	    fatal_dump("MAIN: Internal error -- this should never happen.");
	    break;
	}
    }
    /* NOTREACHED */
    return 0;
}

static void
sendSignal(void)
{
    pid_t pid;
    debug_log = stderr;
    pid = readPidFile();
    if (pid > 1) {
	if (kill(pid, opt_send_signal) &&
	/* ignore permissions if just running check */
	    !(opt_send_signal == 0 && errno == EPERM)) {
	    fprintf(stderr, "%s: ERROR: Could not send ", appname);
	    fprintf(stderr, "signal %d to process %d: %s\n",
		opt_send_signal, (int) pid, xstrerror());
	    exit(1);
	}
    } else {
	fprintf(stderr, "%s: ERROR: No running copy\n", appname);
	exit(1);
    }
    /* signal successfully sent */
    exit(0);
}

/*
 * This function is run when Squid is in daemon mode, just
 * before the parent forks and starts up the child process.
 * It can be used for admin-specific tasks, such as notifying
 * someone that Squid is (re)started.
 */
static void
mainStartScript(const char *prog)
{
    char script[SQUID_MAXPATHLEN];
    char *t;
    size_t sl = 0;
    pid_t cpid;
    pid_t rpid;
    xstrncpy(script, prog, MAXPATHLEN);
    if ((t = strrchr(script, '/'))) {
	*(++t) = '\0';
	sl = strlen(script);
    }
    xstrncpy(&script[sl], squid_start_script, MAXPATHLEN - sl);
    if ((cpid = fork()) == 0) {
	/* child */
	execl(script, squid_start_script, NULL);
	_exit(0);
    } else {
	do {
#ifdef _SQUID_NEXT_
	    union wait status;
	    rpid = wait3(&status, 0, NULL);
#else
	    int status;
	    rpid = waitpid(-1, &status, 0);
#endif
	} while (rpid != cpid);
    }
}

static int
checkRunningPid(void)
{
    pid_t pid;
    debug_log = stderr;
    if (strcmp(Config.pidFilename, "none") == 0) {
	debug(0, 1) ("No pid_filename specified. Trusting you know what you are doing.\n");
	return 0;
    }
    pid = readPidFile();
    if (pid < 2)
	return 0;
    if (kill(pid, 0) < 0)
	return 0;
    debug(0, 0) ("Squid is already running!  Process ID %ld\n", (long int) pid);
    return 1;
}

static void
watch_child(char *argv[])
{
    char *prog;
    int failcount = 0;
    time_t start;
    time_t stop;
#ifdef _SQUID_NEXT_
    union wait status;
#else
    int status;
#endif
    pid_t pid;
#ifdef TIOCNOTTY
    int i;
#endif
    int nullfd;
    if (*(argv[0]) == '(')
	return;
    openlog(appname, LOG_PID | LOG_NDELAY | LOG_CONS, LOG_LOCAL4);
    if ((pid = fork()) < 0)
	syslog(LOG_ALERT, "fork failed: %s", xstrerror());
    else if (pid > 0)
	exit(0);
    if (setsid() < 0)
	syslog(LOG_ALERT, "setsid failed: %s", xstrerror());
    closelog();
#ifdef TIOCNOTTY
    if ((i = open("/dev/tty", O_RDWR | O_TEXT)) >= 0) {
	ioctl(i, TIOCNOTTY, NULL);
	close(i);
    }
#endif


    /*
     * RBCOLLINS - if cygwin stackdumps when squid is run without
     * -N, check the cygwin1.dll version, it needs to be AT LEAST
     * 1.1.3.  execvp had a bit overflow error in a loop..
     */
    /* Connect stdio to /dev/null in daemon mode */
    nullfd = open("/dev/null", O_RDWR | O_TEXT);
    if (nullfd < 0)
	fatalf("/dev/null: %s\n", xstrerror());
    dup2(nullfd, 0);
    if (opt_debug_stderr < 0) {
	dup2(nullfd, 1);
	dup2(nullfd, 2);
    }
    for (;;) {
	mainStartScript(argv[0]);
	if ((pid = fork()) == 0) {
	    /* child */
	    openlog(appname, LOG_PID | LOG_NDELAY | LOG_CONS, LOG_LOCAL4);
	    prog = xstrdup(argv[0]);
	    argv[0] = xstrdup("(squid)");
	    execvp(prog, argv);
	    syslog(LOG_ALERT, "execvp failed: %s", xstrerror());
	}
	/* parent */
	openlog(appname, LOG_PID | LOG_NDELAY | LOG_CONS, LOG_LOCAL4);
	syslog(LOG_NOTICE, "Squid Parent: child process %d started", pid);
	time(&start);
	squid_signal(SIGINT, SIG_IGN, SA_RESTART);
#ifdef _SQUID_NEXT_
	pid = wait3(&status, 0, NULL);
#else
	pid = waitpid(-1, &status, 0);
#endif
	time(&stop);
	if (WIFEXITED(status)) {
	    syslog(LOG_NOTICE,
		"Squid Parent: child process %d exited with status %d",
		pid, WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
	    syslog(LOG_NOTICE,
		"Squid Parent: child process %d exited due to signal %d",
		pid, WTERMSIG(status));
	} else {
	    syslog(LOG_NOTICE, "Squid Parent: child process %d exited", pid);
	}
	if (stop - start < 10)
	    failcount++;
	else
	    failcount = 0;
	if (failcount == 5) {
	    syslog(LOG_ALERT, "Exiting due to repeated, frequent failures");
	    exit(1);
	}
	if (WIFEXITED(status))
	    if (WEXITSTATUS(status) == 0)
		exit(0);
	if (WIFSIGNALED(status)) {
	    switch (WTERMSIG(status)) {
	    case SIGKILL:
		exit(0);
		break;
	    default:
		break;
	    }
	}
	squid_signal(SIGINT, SIG_DFL, SA_RESTART);
	sleep(3);
    }
    /* NOTREACHED */
}

static void
SquidShutdown(void *unused)
{
    debug(1, 1) ("Shutting down...\n");
#if USE_DNSSERVERS
    dnsShutdown();
#else
    idnsShutdown();
#endif
    redirectShutdown();
    externalAclShutdown();
    icpConnectionClose();
#if USE_HTCP
    htcpSocketClose();
#endif
#ifdef SQUID_SNMP
    snmpConnectionClose();
#endif
#if USE_WCCP
    wccpConnectionClose();
#endif
    releaseServerSockets();
    commCloseAllSockets();
    authenticateShutdown();
#if USE_UNLINKD
    unlinkdClose();
#endif
    storeDirSync();		/* Flush pending object writes/unlinks */
    storeDirWriteCleanLogs(0);
    PrintRusage();
    dumpMallocStats();
    storeDirSync();		/* Flush log writes */
    storeLogClose();
    accessLogClose();
    useragentLogClose();
    refererCloseLog();
#if WIP_FWD_LOG
    fwdUninit();
#endif
    storeDirSync();		/* Flush log close */
    storeFsDone();
#if PURIFY || XMALLOC_TRACE
    configFreeMemory();
    storeFreeMemory();
    /*stmemFreeMemory(); */
    netdbFreeMemory();
    ipcacheFreeMemory();
    fqdncacheFreeMemory();
    asnFreeMemory();
    clientdbFreeMemory();
    httpHeaderCleanModule();
    statFreeMemory();
    eventFreeMemory();
    mimeFreeMemory();
    errorClean();
#endif
#if !XMALLOC_TRACE
    if (opt_no_daemon) {
	fd_close(0);
	fd_close(1);
	fd_close(2);
    }
#endif
    comm_select_shutdown();
    fdDumpOpen();
    fdFreeMemory();
    memClean();
#if XMALLOC_TRACE
    xmalloc_find_leaks();
    debug(1, 0) ("Memory used after shutdown: %d\n", xmalloc_total);
#endif
#if MEM_GEN_TRACE
    log_trace_done();
#endif
    if (Config.pidFilename && strcmp(Config.pidFilename, "none") != 0) {
	enter_suid();
	safeunlink(Config.pidFilename, 0);
	leave_suid();
    }
    debug(1, 1) ("Squid Cache (Version %s): Exiting normally.\n",
	version_string);
    if (debug_log)
	fclose(debug_log);
    exit(0);
}
