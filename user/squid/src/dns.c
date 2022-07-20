
/*
 * $Id: dns.c,v 1.86.2.2 2003/12/06 17:19:37 hno Exp $
 *
 * DEBUG: section 34    Dnsserver interface
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

static helper *dnsservers = NULL;

static void
dnsStats(StoreEntry * sentry)
{
    storeAppendPrintf(sentry, "Dnsserver Statistics:\n");
    helperStats(sentry, dnsservers);
}

void
dnsInit(void)
{
    static int init = 0;
    wordlist *w;
    if (!Config.Program.dnsserver)
	return;
    if (dnsservers == NULL)
	dnsservers = helperCreate("dnsserver");
    dnsservers->n_to_start = Config.dnsChildren;
    dnsservers->ipc_type = IPC_TCP_SOCKET;
    assert(dnsservers->cmdline == NULL);
    wordlistAdd(&dnsservers->cmdline, Config.Program.dnsserver);
    if (Config.onoff.res_defnames)
	wordlistAdd(&dnsservers->cmdline, "-D");
    for (w = Config.dns_nameservers; w != NULL; w = w->next) {
	wordlistAdd(&dnsservers->cmdline, "-s");
	wordlistAdd(&dnsservers->cmdline, w->key);
    }
    helperOpenServers(dnsservers);
    if (!init) {
	cachemgrRegister("dns",
	    "Dnsserver Statistics",
	    dnsStats, 0, 1);
	init = 1;
    }
}

void
dnsShutdown(void)
{
    if (!dnsservers)
	return;
    helperShutdown(dnsservers);
    wordlistDestroy(&dnsservers->cmdline);
    if (!shutting_down)
	return;
    helperFree(dnsservers);
    dnsservers = NULL;
}

void
dnsSubmit(const char *lookup, HLPCB * callback, void *data)
{
    char buf[256];
    static time_t first_warn = 0;
    snprintf(buf, 256, "%s\n", lookup);
    if (dnsservers->stats.queue_size >= dnsservers->n_running * 2) {
	if (first_warn == 0)
	    first_warn = squid_curtime;
	if (squid_curtime - first_warn > 3 * 60)
	    fatal("DNS servers not responding for 3 minutes");
	debug(34, 1) ("dnsSubmit: queue overload, rejecting %s\n", lookup);
	callback(data, (char *) "$fail Temporary network problem, please retry later");
	return;
    }
    first_warn = 0;
    helperSubmit(dnsservers, buf, callback, data);
}

#ifdef SQUID_SNMP
/*
 * The function to return the DNS via SNMP
 */
variable_list *
snmp_netDnsFn(variable_list * Var, snint * ErrP)
{
    variable_list *Answer = NULL;
    debug(49, 5) ("snmp_netDnsFn: Processing request: %d\n", Var->name[LEN_SQ_NET + 1]);
    snmpDebugOid(5, Var->name, Var->name_length);
    *ErrP = SNMP_ERR_NOERROR;
    switch (Var->name[LEN_SQ_NET + 1]) {
    case DNS_REQ:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    dnsservers->stats.requests,
	    SMI_COUNTER32);
	break;
    case DNS_REP:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    dnsservers->stats.replies,
	    SMI_COUNTER32);
	break;
    case DNS_SERVERS:
	Answer = snmp_var_new_integer(Var->name, Var->name_length,
	    dnsservers->n_running,
	    SMI_COUNTER32);
	break;
    default:
	*ErrP = SNMP_ERR_NOSUCHNAME;
	break;
    }
    return Answer;
}
#endif /*SQUID_SNMP */
