
/*
 * $Id: defines.h,v 1.97.2.4 2005/03/26 02:50:52 hno Exp $
 *
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

#ifndef SQUID_DEFINES_H
#define SQUID_DEFINES_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ACL_NAME_SZ 32
#define BROWSERNAMELEN 128

#define ACL_SUNDAY	0x01
#define ACL_MONDAY	0x02
#define ACL_TUESDAY	0x04
#define ACL_WEDNESDAY	0x08
#define ACL_THURSDAY	0x10
#define ACL_FRIDAY	0x20
#define ACL_SATURDAY	0x40
#define ACL_ALLWEEK	0x7F
#define ACL_WEEKDAYS	0x3E

#define MAXHTTPPORTS			128

#define COMM_OK		  (0)
#define COMM_ERROR	 (-1)
#define COMM_NOMESSAGE	 (-3)
#define COMM_TIMEOUT	 (-4)
#define COMM_SHUTDOWN	 (-5)
#define COMM_INPROGRESS  (-6)
#define COMM_ERR_CONNECT (-7)
#define COMM_ERR_DNS     (-8)
#define COMM_ERR_CLOSING (-9)

/* Select types. */
#define COMM_SELECT_READ   (0x1)
#define COMM_SELECT_WRITE  (0x2)
#define MAX_DEBUG_SECTIONS 100

#define COMM_NONBLOCKING	0x01
#define COMM_NOCLOEXEC		0x02
#define COMM_REUSEADDR		0x04

#define debug(SECTION, LEVEL) \
        ((_db_level = (LEVEL)) > debugLevels[SECTION]) ? (void) 0 : _db_print

#define safe_free(x)	if (x) { xxfree(x); x = NULL; }

#define DISK_OK                   (0)
#define DISK_ERROR               (-1)
#define DISK_EOF                 (-2)
#define DISK_NO_SPACE_LEFT       (-6)

#define DNS_INBUF_SZ 4096

#define FD_DESC_SZ		64

#define FQDN_LOOKUP_IF_MISS	0x01
#define FQDN_MAX_NAMES 5

#define HTTP_REPLY_FIELD_SZ 128

#define BUF_TYPE_8K 	1
#define BUF_TYPE_MALLOC 2

#define ANONYMIZER_NONE		0
#define ANONYMIZER_STANDARD	1
#define ANONYMIZER_PARANOID	2

#define USER_IDENT_SZ 64
#define IDENT_NONE 0
#define IDENT_PENDING 1
#define IDENT_DONE 2

#define IP_LOOKUP_IF_MISS	0x01

#define MAX_MIME 4096

/* Mark a neighbor cache as dead if it doesn't answer this many pings */
#define HIER_MAX_DEFICIT  20

#define ICP_FLAG_HIT_OBJ     0x80000000ul
#define ICP_FLAG_SRC_RTT     0x40000000ul

/* Version */
#define ICP_VERSION_2		2
#define ICP_VERSION_3		3
#define ICP_VERSION_CURRENT	ICP_VERSION_2

#define DIRECT_UNKNOWN 0
#define DIRECT_NO    1
#define DIRECT_MAYBE 2
#define DIRECT_YES   3

#define REDIRECT_AV_FACTOR 1000

#define REDIRECT_NONE 0
#define REDIRECT_PENDING 1
#define REDIRECT_DONE 2

#define AUTHENTICATE_AV_FACTOR 1000
/* AUTHENTICATION */

#define NTLM_CHALLENGE_SZ 300

#define  CONNECT_PORT        443

#define current_stacksize(stack) ((stack)->top - (stack)->base)

/* logfile status */
#define LOG_ENABLE  1
#define LOG_DISABLE 0

#define SM_PAGE_SIZE 4096

#define EBIT_SET(flag, bit) 	((void)((flag) |= ((1L<<(bit)))))
#define EBIT_CLR(flag, bit) 	((void)((flag) &= ~((1L<<(bit)))))
#define EBIT_TEST(flag, bit) 	((flag) & ((1L<<(bit))))

/* bit opearations on a char[] mask of unlimited length */
#define CBIT_BIT(bit)           (1<<((bit)%8))
#define CBIT_BIN(mask, bit)     (mask)[(bit)>>3]
#define CBIT_SET(mask, bit) 	((void)(CBIT_BIN(mask, bit) |= CBIT_BIT(bit)))
#define CBIT_CLR(mask, bit) 	((void)(CBIT_BIN(mask, bit) &= ~CBIT_BIT(bit)))
#define CBIT_TEST(mask, bit) 	(CBIT_BIN(mask, bit) & CBIT_BIT(bit))

#define MAX_FILES_PER_DIR (1<<20)

#define MAX_URL  4096
#define MAX_LOGIN_SZ  128

#define PEER_MAX_ADDRESSES 10
#define RTT_AV_FACTOR      50

#define PEER_DEAD 0
#define PEER_ALIVE 1

#define AUTH_MSG_SZ 4096
#define HTTP_REPLY_BUF_SZ 4096
#define CLIENT_REQ_BUF_SZ 4096

#if !defined(ERROR_BUF_SZ) && defined(MAX_URL)
#define ERROR_BUF_SZ (MAX_URL << 2)
#endif

#define READ_AHEAD_GAP		(1<<14)

#if SQUID_SNMP
#define VIEWINCLUDED    1
#define VIEWEXCLUDED    2
#endif

#define STORE_META_OK     0x03
#define STORE_META_DIRTY  0x04
#define STORE_META_BAD    0x05

#define IPC_NONE 0
#define IPC_TCP_SOCKET 1
#define IPC_UDP_SOCKET 2
#define IPC_FIFO 3

#define STORE_META_KEY STORE_META_KEY_MD5

#define STORE_META_TLD_START sizeof(int)+sizeof(char)
#define STORE_META_TLD_SIZE STORE_META_TLD_START
#define SwapMetaType(x) (char)x[0]
#define SwapMetaSize(x) &x[sizeof(char)]
#define SwapMetaData(x) &x[STORE_META_TLD_START]
#define STORE_HDR_METASIZE (4*sizeof(time_t)+2*sizeof(u_short)+sizeof(squid_file_sz))
#define STORE_HDR_METASIZE_OLD (4*sizeof(time_t)+2*sizeof(u_short)+sizeof(size_t))

#define STORE_ENTRY_WITH_MEMOBJ		1
#define STORE_ENTRY_WITHOUT_MEMOBJ	0

#define PINGER_PAYLOAD_SZ 8192

#define COUNT_INTERVAL 60
/*
 * keep 60 minutes' worth of per-minute readings (+ current reading)
 */
#define N_COUNT_HIST (3600 / COUNT_INTERVAL) + 1
/*
 * keep 3 days' (72 hours) worth of hourly readings
 */
#define N_COUNT_HOUR_HIST (86400 * 3) / (60 * COUNT_INTERVAL)

/* were to look for errors if config path fails */
#ifndef DEFAULT_SQUID_ERROR_DIR
#define DEFAULT_SQUID_ERROR_DIR "/usr/local/squid/etc/errors"
#endif

/* gb_type operations */
#define gb_flush_limit (0x3FFFFFFF)
#define gb_inc(gb, delta) { if ((gb)->bytes > gb_flush_limit || delta > gb_flush_limit) gb_flush(gb); (gb)->bytes += delta; (gb)->count++; }

/* iteration for HttpHdrRange */
#define HttpHdrRangeInitPos (-1)

/* use this and only this to initialize HttpHeaderPos */
#define HttpHeaderInitPos (-1)

/* handy to determine the #elements in a static array */
#define countof(arr) (sizeof(arr)/sizeof(*arr))

/* to initialize static variables (see also MemBufNull) */
#define MemBufNULL { NULL, 0, 0, 0, NULL }

/*
 * Max number of ICP messages to receive per call to icpHandleUdp
 */
#ifdef _SQUID_MSWIN_
#define INCOMING_ICP_MAX 1
#else
#define INCOMING_ICP_MAX 15
#endif
/*
 * Max number of DNS messages to receive per call to DNS read handler
 */
#ifdef _SQUID_MSWIN_
#define INCOMING_DNS_MAX 1
#else
#define INCOMING_DNS_MAX 15
#endif
/*
 * Max number of HTTP connections to accept per call to httpAccept
 * and PER HTTP PORT
 */
#ifdef _SQUID_MSWIN_
#define INCOMING_HTTP_MAX 1
#else
#define INCOMING_HTTP_MAX 10
#endif
#define INCOMING_TOTAL_MAX (INCOMING_ICP_MAX+INCOMING_HTTP_MAX)

/*
 * This many TCP connections must FAIL before we mark the
 * peer as DEAD
 */
#define PEER_TCP_MAGIC_COUNT 10

#define CLIENT_SOCK_SZ 4096

#define URI_WHITESPACE_STRIP 0
#define URI_WHITESPACE_ALLOW 1
#define URI_WHITESPACE_ENCODE 2
#define URI_WHITESPACE_CHOP 3
#define URI_WHITESPACE_DENY 4

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

/* cbdata macros */
#define cbdataAlloc(type) ((type *)cbdataInternalAlloc(CBDATA_##type))
#define cbdataFree(var) (var = (var != NULL ? cbdataInternalFree(var): NULL))
#define CBDATA_TYPE(type)	static cbdata_type CBDATA_##type = 0
#define CBDATA_GLOBAL_TYPE(type)	cbdata_type CBDATA_##type
#define CBDATA_INIT_TYPE(type)	(CBDATA_##type ? 0 : (CBDATA_##type = cbdataAddType(CBDATA_##type, #type, sizeof(type), NULL)))
#define CBDATA_INIT_TYPE_FREECB(type, free_func)	(CBDATA_##type ? 0 : (CBDATA_##type = cbdataAddType(CBDATA_##type, #type, sizeof(type), free_func)))

#ifndef O_TEXT
#define O_TEXT 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * Macro to find file access mode
 */
#ifdef O_ACCMODE
#define FILE_MODE(x) ((x)&O_ACCMODE)
#else
#define FILE_MODE(x) ((x)&(O_RDONLY|O_WRONLY|O_RDWR))
#endif

#endif /* SQUID_DEFINES_H */
