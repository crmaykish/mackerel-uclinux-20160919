
/*
 * $Id: enums.h,v 1.203.2.15 2005/03/26 02:50:52 hno Exp $
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

#ifndef SQUID_ENUMS_H
#define SQUID_ENUMS_H

typedef enum {
    LOG_TAG_NONE,
    LOG_TCP_HIT,
    LOG_TCP_MISS,
    LOG_TCP_REFRESH_HIT,
    LOG_TCP_REFRESH_FAIL_HIT,
    LOG_TCP_REFRESH_MISS,
    LOG_TCP_CLIENT_REFRESH_MISS,
    LOG_TCP_IMS_HIT,
    LOG_TCP_SWAPFAIL_MISS,
    LOG_TCP_NEGATIVE_HIT,
    LOG_TCP_MEM_HIT,
    LOG_TCP_DENIED,
    LOG_TCP_OFFLINE_HIT,
#if LOG_TCP_REDIRECTS
    LOG_TCP_REDIRECT,
#endif
    LOG_UDP_HIT,
    LOG_UDP_MISS,
    LOG_UDP_DENIED,
    LOG_UDP_INVALID,
    LOG_UDP_MISS_NOFETCH,
    LOG_ICP_QUERY,
    LOG_TYPE_MAX
} log_type;

typedef enum {
    ERR_NONE,
    ERR_READ_TIMEOUT,
    ERR_LIFETIME_EXP,
    ERR_READ_ERROR,
    ERR_WRITE_ERROR,
    ERR_SHUTTING_DOWN,
    ERR_CONNECT_FAIL,
    ERR_INVALID_REQ,
    ERR_UNSUP_REQ,
    ERR_INVALID_URL,
    ERR_SOCKET_FAILURE,
    ERR_DNS_FAIL,
    ERR_CANNOT_FORWARD,
    ERR_FORWARDING_DENIED,
    ERR_NO_RELAY,
    ERR_ZERO_SIZE_OBJECT,
    ERR_FTP_DISABLED,
    ERR_FTP_FAILURE,
    ERR_URN_RESOLVE,
    ERR_ACCESS_DENIED,
    ERR_CACHE_ACCESS_DENIED,
    ERR_CACHE_MGR_ACCESS_DENIED,
    ERR_SQUID_SIGNATURE,	/* not really an error */
    ERR_FTP_PUT_CREATED,	/* !error,a note that the file was created */
    ERR_FTP_PUT_MODIFIED,	/* modified, !created */
    ERR_FTP_PUT_ERROR,
    ERR_FTP_NOT_FOUND,
    ERR_FTP_FORBIDDEN,
    ERR_FTP_UNAVAILABLE,
    ERR_ONLY_IF_CACHED_MISS,	/* failure to satisfy only-if-cached request */
    ERR_TOO_BIG,
    TCP_RESET,
    ERR_ICAP_FAILURE,
    ERR_INVALID_RESP,
    ERR_MAX
} err_type;

typedef enum {
    ACL_NONE,
    ACL_SRC_IP,
    ACL_DST_IP,
    ACL_MY_IP,
    ACL_SRC_DOMAIN,
    ACL_DST_DOMAIN,
    ACL_SRC_DOM_REGEX,
    ACL_DST_DOM_REGEX,
    ACL_TIME,
    ACL_URLPATH_REGEX,
    ACL_URL_REGEX,
    ACL_URL_PORT,
    ACL_MY_PORT,
#if USE_IDENT
    ACL_IDENT,
    ACL_IDENT_REGEX,
#endif
    ACL_PROTO,
    ACL_METHOD,
    ACL_BROWSER,
    ACL_REFERER_REGEX,
    ACL_PROXY_AUTH,
    ACL_PROXY_AUTH_REGEX,
    ACL_SRC_ASN,
    ACL_DST_ASN,
#if USE_ARP_ACL
    ACL_SRC_ARP,
#endif
#if SQUID_SNMP
    ACL_SNMP_COMMUNITY,
#endif
#if SRC_RTT_NOT_YET_FINISHED
    ACL_NETDB_SRC_RTT,
#endif
    ACL_MAXCONN,
    ACL_REQ_MIME_TYPE,
    ACL_REP_MIME_TYPE,
    ACL_REP_HEADER,
    ACL_REQ_HEADER,
    ACL_MAX_USER_IP,
    ACL_EXTERNAL,
    ACL_URLLOGIN,
    ACL_ENUM_MAX
} squid_acl;

typedef enum {
    ACL_LOOKUP_NONE,
    ACL_LOOKUP_NEEDED,
    ACL_LOOKUP_PENDING,
    ACL_LOOKUP_DONE,
    ACL_PROXY_AUTH_NEEDED
} acl_lookup_state;

enum {
    FD_NONE,
    FD_LOG,
    FD_FILE,
    FD_SOCKET,
    FD_PIPE,
    FD_UNKNOWN
};

enum {
    FD_READ,
    FD_WRITE
};

typedef enum {
    PEER_NONE,
    PEER_SIBLING,
    PEER_PARENT,
    PEER_MULTICAST
} peer_t;

typedef enum {
    LOOKUP_NONE,
    LOOKUP_HIT,
    LOOKUP_MISS
} lookup_t;

/* recognized or "known" header fields; @?@ add more! */
typedef enum {
    HDR_ACCEPT,
    HDR_ACCEPT_CHARSET,
    HDR_ACCEPT_ENCODING,
    HDR_ACCEPT_LANGUAGE,
    HDR_ACCEPT_RANGES,
    HDR_AGE,
    HDR_ALLOW,
    HDR_AUTHORIZATION,
    HDR_CACHE_CONTROL,
    HDR_CONNECTION,
    HDR_CONTENT_BASE,
    HDR_CONTENT_DISPOSITION,
    HDR_CONTENT_ENCODING,
    HDR_CONTENT_LANGUAGE,
    HDR_CONTENT_LENGTH,
    HDR_CONTENT_LOCATION,
    HDR_CONTENT_MD5,
    HDR_CONTENT_RANGE,
    HDR_CONTENT_TYPE,
    HDR_COOKIE,
    HDR_DATE,
    HDR_ETAG,
    HDR_EXPIRES,
    HDR_FROM,
    HDR_HOST,
    HDR_IF_MATCH,
    HDR_IF_MODIFIED_SINCE,
    HDR_IF_NONE_MATCH,
    HDR_IF_RANGE,
    HDR_LAST_MODIFIED,
    HDR_LINK,
    HDR_LOCATION,
    HDR_MAX_FORWARDS,
    HDR_MIME_VERSION,
    HDR_PRAGMA,
    HDR_PROXY_AUTHENTICATE,
    HDR_PROXY_AUTHENTICATION_INFO,
    HDR_PROXY_AUTHORIZATION,
    HDR_PROXY_CONNECTION,
    HDR_PUBLIC,
    HDR_RANGE,
    HDR_REQUEST_RANGE,		/* some clients use this, sigh */
    HDR_REFERER,
    HDR_RETRY_AFTER,
    HDR_SERVER,
    HDR_SET_COOKIE,
    HDR_TITLE,
    HDR_TRANSFER_ENCODING,
    HDR_UPGRADE,
    HDR_USER_AGENT,
    HDR_VARY,
    HDR_VIA,
    HDR_WARNING,
    HDR_WWW_AUTHENTICATE,
    HDR_AUTHENTICATION_INFO,
    HDR_X_CACHE,
    HDR_X_CACHE_LOOKUP,		/* tmp hack, remove later */
    HDR_X_FORWARDED_FOR,
    HDR_X_REQUEST_URI,		/* appended if ADD_X_REQUEST_URI is #defined */
    HDR_X_SQUID_ERROR,
    HDR_NEGOTIATE,
#if X_ACCELERATOR_VARY
    HDR_X_ACCELERATOR_VARY,
#endif
    HDR_OTHER,
    HDR_ENUM_END
} http_hdr_type;

typedef enum {
    CC_PUBLIC,
    CC_PRIVATE,
    CC_NO_CACHE,
    CC_NO_STORE,
    CC_NO_TRANSFORM,
    CC_MUST_REVALIDATE,
    CC_PROXY_REVALIDATE,
    CC_MAX_AGE,
    CC_S_MAXAGE,
    CC_MAX_STALE,
    CC_ONLY_IF_CACHED,
    CC_OTHER,
    CC_ENUM_END
} http_hdr_cc_type;

/* possible types for http header fields */
typedef enum {
    ftInvalid = HDR_ENUM_END,	/* to catch nasty errors with hdr_id<->fld_type clashes */
    ftInt,
    ftStr,
    ftDate_1123,
    ftETag,
    ftPCc,
    ftPContRange,
    ftPRange,
    ftDate_1123_or_ETag,
    ftSize
} field_type;

/* possible owners of http header */
typedef enum {
    hoNone,
#if USE_HTCP
    hoHtcpReply,
#endif
    hoRequest,
    hoReply
} http_hdr_owner_type;

typedef enum {
    HIER_NONE,
    DIRECT,
    SIBLING_HIT,
    PARENT_HIT,
    DEFAULT_PARENT,
    SINGLE_PARENT,
    FIRSTUP_PARENT,
    FIRST_PARENT_MISS,
    CLOSEST_PARENT_MISS,
    CLOSEST_PARENT,
    CLOSEST_DIRECT,
    NO_DIRECT_FAIL,
    SOURCE_FASTEST,
    ROUNDROBIN_PARENT,
#if USE_CACHE_DIGESTS
    CD_PARENT_HIT,
    CD_SIBLING_HIT,
#endif
#if USE_CARP
    CARP,
#endif
    ANY_OLD_PARENT,
    HIER_MAX
} hier_code;

typedef enum {
    ICP_INVALID,
    ICP_QUERY,
    ICP_HIT,
    ICP_MISS,
    ICP_ERR,
    ICP_SEND,
    ICP_SENDA,
    ICP_DATABEG,
    ICP_DATA,
    ICP_DATAEND,
    ICP_SECHO,
    ICP_DECHO,
    ICP_NOTIFY,
    ICP_INVALIDATE,
    ICP_DELETE,
    ICP_UNUSED15,
    ICP_UNUSED16,
    ICP_UNUSED17,
    ICP_UNUSED18,
    ICP_UNUSED19,
    ICP_UNUSED20,
    ICP_MISS_NOFETCH,
    ICP_DENIED,
    ICP_HIT_OBJ,
    ICP_END
} icp_opcode;

enum {
    NOT_IN_MEMORY,
    IN_MEMORY
};

enum {
    PING_NONE,
    PING_WAITING,
    PING_DONE
};

enum {
    STORE_OK,
    STORE_PENDING
};

enum {
    SWAPOUT_NONE,
    SWAPOUT_WRITING,
    SWAPOUT_DONE
};

typedef enum {
    STORE_NON_CLIENT,
    STORE_MEM_CLIENT,
    STORE_DISK_CLIENT
} store_client_t;

enum {
    METHOD_NONE,		/* 000 */
    METHOD_GET,			/* 001 */
    METHOD_POST,		/* 010 */
    METHOD_PUT,			/* 011 */
    METHOD_HEAD,		/* 100 */
    METHOD_CONNECT,		/* 101 */
    METHOD_TRACE,		/* 110 */
    METHOD_PURGE,		/* 111 */
    METHOD_OPTIONS,
    METHOD_DELETE,		/* RFC2616 section 9.7 */
    METHOD_PROPFIND,
    METHOD_PROPPATCH,
    METHOD_MKCOL,
    METHOD_COPY,
    METHOD_MOVE,
    METHOD_LOCK,
    METHOD_UNLOCK,
    METHOD_BMOVE,
    METHOD_BDELETE,
    METHOD_BPROPFIND,
    METHOD_BPROPPATCH,
    METHOD_BCOPY,
    METHOD_SEARCH,
    METHOD_SUBSCRIBE,
    METHOD_UNSUBSCRIBE,
    METHOD_POLL,
    METHOD_EXT00,
    METHOD_EXT01,
    METHOD_EXT02,
    METHOD_EXT03,
    METHOD_EXT04,
    METHOD_EXT05,
    METHOD_EXT06,
    METHOD_EXT07,
    METHOD_EXT08,
    METHOD_EXT09,
    METHOD_EXT10,
    METHOD_EXT11,
    METHOD_EXT12,
    METHOD_EXT13,
    METHOD_EXT14,
    METHOD_EXT15,
    METHOD_EXT16,
    METHOD_EXT17,
    METHOD_EXT18,
    METHOD_EXT19,
    METHOD_ENUM_END
};
typedef unsigned int method_t;

typedef enum {
    PROTO_NONE,
    PROTO_HTTP,
    PROTO_FTP,
    PROTO_GOPHER,
    PROTO_WAIS,
    PROTO_CACHEOBJ,
    PROTO_ICP,
#if USE_HTCP
    PROTO_HTCP,
#endif
    PROTO_URN,
    PROTO_WHOIS,
    PROTO_INTERNAL,
    PROTO_HTTPS,
#if HS_FEAT_ICAP
    PROTO_ICAP,
#endif
    PROTO_MAX
} protocol_t;

typedef enum {
    HTTP_STATUS_NONE = 0,
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_PROCESSING = 102,	/* RFC2518 section 10.1 */
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_MULTI_STATUS = 207,	/* RFC2518 section 10.2 */
    HTTP_MULTIPLE_CHOICES = 300,
    HTTP_MOVED_PERMANENTLY = 301,
    HTTP_MOVED_TEMPORARILY = 302,
    HTTP_SEE_OTHER = 303,
    HTTP_NOT_MODIFIED = 304,
    HTTP_USE_PROXY = 305,
    HTTP_TEMPORARY_REDIRECT = 307,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_PAYMENT_REQUIRED = 402,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_REQUEST_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PRECONDITION_FAILED = 412,
    HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_REQUEST_URI_TOO_LARGE = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_UNPROCESSABLE_ENTITY = 422,	/* RFC2518 section 10.3 */
    HTTP_LOCKED = 423,		/* RFC2518 section 10.4 */
    HTTP_FAILED_DEPENDENCY = 424,	/* RFC2518 section 10.5 */
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_INSUFFICIENT_STORAGE = 507,	/* RFC2518 section 10.6 */
    HTTP_INVALID_HEADER = 600,	/* Squid header parsing error */
    HTTP_HEADER_TOO_LARGE = 601		/* Header too large to process */
} http_status;

/*
 * These are for StoreEntry->flag, which is defined as a SHORT
 *
 * NOTE: These flags are written to swap.state, so think very carefully
 * about deleting or re-assigning!
 */
enum {
    ENTRY_SPECIAL,
    ENTRY_REVALIDATE,
    DELAY_SENDING,
    RELEASE_REQUEST,
    REFRESH_REQUEST,
    ENTRY_CACHABLE,
    ENTRY_DISPATCHED,
    KEY_PRIVATE,
    ENTRY_FWD_HDR_WAIT,
    ENTRY_NEGCACHED,
    ENTRY_VALIDATED,
    ENTRY_BAD_LENGTH,
    ENTRY_ABORTED
#if UNUSED_CODE
    ENTRY_DONT_LOG
#endif
};

typedef enum {
    ACCESS_DENIED,
    ACCESS_ALLOWED,
    ACCESS_REQ_PROXY_AUTH
} allow_t;

typedef enum {
    AUTH_ACL_CHALLENGE = -2,
    AUTH_ACL_HELPER = -1,
    AUTH_ACL_CANNOT_AUTHENTICATE = 0,
    AUTH_AUTHENTICATED = 1
} auth_acl_t;

typedef enum {
    AUTH_UNKNOWN,		/* default */
    AUTH_BASIC,
    AUTH_NTLM,
    AUTH_DIGEST,
    AUTH_BROKEN			/* known type, but broken data */
} auth_type_t;

/* stateful helper reservation info */
typedef enum {
    S_HELPER_FREE,		/* available for requests */
    S_HELPER_RESERVED,		/* in a reserved state - no active request, but state data in the helper shouldn't be disturbed */
    S_HELPER_DEFERRED		/* available for requests, and at least one more will come from a previous caller with the server pointer */
} stateful_helper_reserve_t;


#if SQUID_SNMP
enum {
    SNMP_C_VIEW,
    SNMP_C_USER,
    SNMP_C_COMMUNITY
};

#endif

typedef enum {
    MEM_NONE,
    MEM_2K_BUF,
    MEM_4K_BUF,
    MEM_8K_BUF,
    MEM_16K_BUF,
    MEM_32K_BUF,
    MEM_64K_BUF,
    MEM_ACL,
    MEM_ACL_DENY_INFO_LIST,
    MEM_ACL_IP_DATA,
    MEM_ACL_LIST,
    MEM_ACL_NAME_LIST,
    MEM_AUTH_USER_T,
    MEM_AUTH_USER_HASH,
    MEM_ACL_PROXY_AUTH_MATCH,
    MEM_ACL_USER_DATA,
    MEM_ACL_TIME_DATA,
#if USE_CACHE_DIGESTS
    MEM_CACHE_DIGEST,
#endif
    MEM_CLIENT_INFO,
    MEM_CLIENT_SOCK_BUF,
    MEM_LINK_LIST,
    MEM_DLINK_NODE,
    MEM_DONTFREE,
    MEM_DREAD_CTRL,
    MEM_DWRITE_Q,
    MEM_FQDNCACHE_ENTRY,
    MEM_FWD_SERVER,
    MEM_HELPER_REQUEST,
    MEM_HELPER_STATEFUL_REQUEST,
    MEM_HTTP_HDR_CC,
    MEM_HTTP_HDR_CONTENT_RANGE,
    MEM_HTTP_HDR_ENTRY,
    MEM_HTTP_HDR_RANGE,
    MEM_HTTP_HDR_RANGE_SPEC,
    MEM_HTTP_REPLY,
    MEM_INTLIST,
    MEM_IPCACHE_ENTRY,
    MEM_MD5_DIGEST,
    MEM_MEMOBJECT,
    MEM_MEM_NODE,
    MEM_NETDBENTRY,
    MEM_NET_DB_NAME,
    MEM_RELIST,
    MEM_REQUEST_T,
    MEM_STOREENTRY,
    MEM_WORDLIST,
#if !USE_DNSSERVERS
    MEM_IDNS_QUERY,
#endif
    MEM_EVENT,
    MEM_TLV,
    MEM_SWAP_LOG_DATA,
    MEM_CLIENT_REQ_BUF,
#if HS_FEAT_ICAP
    MEM_ICAP_OPT_DATA,
    MEM_ICAP_SERVICE_LIST,
    MEM_ICAP_CLASS,
    MEM_ICAP_ACCESS,
#endif
    MEM_MAX
} mem_type;

/*
 * NOTE!  We must preserve the order of this list!
 */
enum {
    STORE_META_VOID,		/* should not come up */
    STORE_META_KEY_URL,		/* key w/ keytype */
    STORE_META_KEY_SHA,
    STORE_META_KEY_MD5,
    STORE_META_URL,		/* the url , if not in the header */
    STORE_META_STD,		/* standard metadata */
    STORE_META_HITMETERING,	/* reserved for hit metering */
    STORE_META_VALID,
    STORE_META_VARY_HEADERS,	/* Stores Vary request headers */
    STORE_META_STD_LFS,		/* standard metadata in lfs format */
    STORE_META_END
};

enum {
    STORE_LOG_CREATE,
    STORE_LOG_SWAPIN,
    STORE_LOG_SWAPOUT,
    STORE_LOG_RELEASE,
    STORE_LOG_SWAPOUTFAIL
};

typedef enum {
    SWAP_LOG_NOP,
    SWAP_LOG_ADD,
    SWAP_LOG_DEL,
    SWAP_LOG_VERSION,
    SWAP_LOG_MAX
} swap_log_op;


/* parse state of HttpReply or HttpRequest */
typedef enum {
    psReadyToParseStartLine = 0,
    psReadyToParseHeaders,
    psParsed,
    psError
} HttpMsgParseState;


enum {
    MEDIAN_HTTP,
    MEDIAN_ICP_QUERY,
    MEDIAN_DNS,
    MEDIAN_HIT,
    MEDIAN_MISS,
    MEDIAN_NM,
    MEDIAN_NH,
    MEDIAN_ICP_REPLY
};

enum {
    SENT,
    RECV
};

/*
 * These are field indicators for raw cache-cache netdb transfers
 */
enum {
    NETDB_EX_NONE,
    NETDB_EX_NETWORK,
    NETDB_EX_RTT,
    NETDB_EX_HOPS
};

/*
 * cbdata types. similar to the MEM_* types above, but managed
 * in cbdata.c. A big difference is that these types are dynamically
 * allocated. This list is only a list of predefined types. Other types
 * are added runtime
 */
typedef enum {
    CBDATA_UNKNOWN = 0,
    CBDATA_UNDEF = 0,
    CBDATA_acl_access,
    CBDATA_aclCheck_t,
    CBDATA_clientHttpRequest,
    CBDATA_ConnStateData,
    CBDATA_ErrorState,
    CBDATA_FwdState,
    CBDATA_generic_cbdata,
    CBDATA_helper,
    CBDATA_helper_server,
    CBDATA_statefulhelper,
    CBDATA_helper_stateful_server,
    CBDATA_HttpStateData,
    CBDATA_peer,
    CBDATA_ps_state,
    CBDATA_RemovalPolicy,
    CBDATA_RemovalPolicyWalker,
    CBDATA_RemovalPurgeWalker,
    CBDATA_store_client,
#ifdef HS_FEAT_ICAP
    CBDATA_IcapStateData,
    CBDATA_icap_service,
#endif
    CBDATA_FIRST_CUSTOM_TYPE = 1000
} cbdata_type;


/*
 * Return codes from checkVary(request)
 */
enum {
    VARY_NONE,
    VARY_MATCH,
    VARY_OTHER,
    VARY_CANCEL
};

/* CygWin & Windows NT Port */
#if defined(_SQUID_MSWIN_) || defined(_SQUID_CYGWIN_)
/*
 * Supported Windows OS types codes
 */
enum {
    _WIN_OS_UNKNOWN,
    _WIN_OS_WIN32S,
    _WIN_OS_WIN95,
    _WIN_OS_WIN98,
    _WIN_OS_WINME,
    _WIN_OS_WINNT,
    _WIN_OS_WIN2K,
    _WIN_OS_WINXP,
    _WIN_OS_WINNET
};

#endif

#if HS_FEAT_ICAP
typedef enum {
    ICAP_STATUS_NONE = 0,
    ICAP_STATUS_CONTINUE = 100,
    ICAP_STATUS_SWITCHING_PROTOCOLS = 101,
    ICAP_STATUS_STATUS_OK = 200,
    ICAP_CREATED = 201,
    ICAP_STATUS_ACCEPTED = 202,
    ICAP_STATUS_NON_AUTHORITATIVE_INFORMATION = 203,
    ICAP_STATUS_NO_MODIFICATION_NEEDED = 204,
    ICAP_STATUS_RESET_CONTENT = 205,
    ICAP_STATUS_PARTIAL_CONTENT = 206,
    ICAP_STATUS_MULTIPLE_CHOICES = 300,
    ICAP_STATUS_MOVED_PERMANENTLY = 301,
    ICAP_STATUS_MOVED_TEMPORARILY = 302,
    ICAP_STATUS_SEE_OTHER = 303,
    ICAP_STATUS_NOT_MODIFIED = 304,
    ICAP_STATUS_USE_PROXY = 305,
    ICAP_STATUS_BAD_REQUEST = 400,
    ICAP_STATUS_UNAUTHORIZED = 401,
    ICAP_STATUS_PAYMENT_REQUIRED = 402,
    ICAP_STATUS_FORBIDDEN = 403,
    ICAP_STATUS_SERVICE_NOT_FOUND = 404,
    ICAP_STATUS_METHOD_NOT_ALLOWED = 405,
    ICAP_STATUS_NOT_ACCEPTABLE = 406,
    ICAP_STATUS_PROXY_AUTHENTICATION_REQUIRED = 407,
    ICAP_STATUS_REQUEST_TIMEOUT = 408,
    ICAP_STATUS_CONFLICT = 409,
    ICAP_STATUS_GONE = 410,
    ICAP_STATUS_LENGTH_REQUIRED = 411,
    ICAP_STATUS_PRECONDITION_FAILED = 412,
    ICAP_STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
    ICAP_STATUS_REQUEST_URI_TOO_LARGE = 414,
    ICAP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
    ICAP_STATUS_INTERNAL_SERVER_ERROR = 500,
    ICAP_STATUS_NOT_IMPLEMENTED = 501,
    ICAP_STATUS_BAD_GATEWAY = 502,
    ICAP_STATUS_SERVICE_OVERLOADED = 503,
    ICAP_STATUS_GATEWAY_TIMEOUT = 504,
    ICAP_STATUS_ICAP_VERSION_NOT_SUPPORTED = 505,
    ICAP_STATUS_INVALID_HEADER = 600
} icap_status;

/*
 * these values are used as index in an array, so it seems to be better to 
 * assign some numbers
 */
typedef enum {
    ICAP_SERVICE_REQMOD_PRECACHE = 0,
    ICAP_SERVICE_REQMOD_POSTCACHE = 1,
    ICAP_SERVICE_RESPMOD_PRECACHE = 2,
    ICAP_SERVICE_RESPMOD_POSTCACHE = 3,
    ICAP_SERVICE_MAX = 4
} icap_service_t;

typedef enum {
    ICAP_METHOD_NONE,
    ICAP_METHOD_OPTION,
    ICAP_METHOD_REQMOD,
    ICAP_METHOD_RESPMOD
} icap_method_t;

#endif /* HS_FEAT_ICAP */

#endif /* SQUID_ENUMS_H */
