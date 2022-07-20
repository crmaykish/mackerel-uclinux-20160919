/* Generic socket.h */
/* $OpenLDAP: pkg/ldap/include/ac/socket.h,v 1.54.2.3 2003/03/05 23:48:31 kurt Exp $ */
/*
 * Copyright 1998-2003 The OpenLDAP Foundation, Redwood City, California, USA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.  A copy of this license is available at
 * http://www.OpenLDAP.org/license.html or in file LICENSE in the
 * top-level directory of the distribution.
 */

#ifndef _AC_SOCKET_H_
#define _AC_SOCKET_H_

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif

#include <netdb.h>

#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif

#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_WINSOCK2
#include <winsock2.h>
#elif HAVE_WINSOCK
#include <winsock.h>
#endif

#ifdef HAVE_PCNFS
#include <tklib.h>
#endif /* HAVE_PCNFS */

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK	(0x7f000001UL)
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif

#undef	sock_errno
#undef	sock_errstr
#define sock_errno()	errno
#define sock_errstr(e)	STRERROR(e)

#ifdef HAVE_WINSOCK
#	define tcp_read( s, buf, len )	recv( s, buf, len, 0 )
#	define tcp_write( s, buf, len )	send( s, buf, len, 0 )
#	define ioctl( s, c, a )		ioctlsocket( (s), (c), (a) )
#	define ioctl_t				u_long
#	define AC_SOCKET_INVALID	((unsigned int) ~0)

#	ifdef SD_BOTH
#		define tcp_close( s )	(shutdown( s, SD_BOTH ), closesocket( s ))
#	else
#		define tcp_close( s )		closesocket( s )
#	endif

#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#define ETIMEDOUT	WSAETIMEDOUT

#undef	sock_errno
#undef	sock_errstr
#define	sock_errno()	WSAGetLastError()
#define	sock_errstr(e)	ber_pvt_wsa_err2string(e)

LBER_F( char * ) ber_pvt_wsa_err2string LDAP_P((int));

#elif MACOS
#	define tcp_close( s )		tcpclose( s )
#	define tcp_read( s, buf, len )	tcpread( s, buf, len )
#	define tcp_write( s, buf, len )	tcpwrite( s, buf, len )

#elif DOS
#	ifdef PCNFS
#		define tcp_close( s )	close( s )
#		define tcp_read( s, buf, len )	recv( s, buf, len, 0 )
#		define tcp_write( s, buf, len )	send( s, buf, len, 0 )
#	endif /* PCNFS */
#	ifdef NCSA
#		define tcp_close( s )	do { netclose( s ); netshut() } while(0)
#		define tcp_read( s, buf, len )	nread( s, buf, len )
#		define tcp_write( s, buf, len )	netwrite( s, buf, len )
#	endif /* NCSA */

#elif HAVE_CLOSESOCKET
#	define tcp_close( s )		closesocket( s )

#	ifdef __BEOS__
#		define tcp_read( s, buf, len )	recv( s, buf, len, 0 )
#		define tcp_write( s, buf, len )	send( s, buf, len, 0 )
#	endif

#else
#	define tcp_read( s, buf, len)	read( s, buf, len )
#	define tcp_write( s, buf, len)	write( s, buf, len )

#	ifdef SHUT_RDWR
#		define tcp_close( s )	(shutdown( s, SHUT_RDWR ), close( s ))
#	else
#		define tcp_close( s )	close( s )
#	endif

#ifdef HAVE_PIPE
/*
 * Only use pipe() on systems where file and socket descriptors
 * are interchangable
 */
#	define USE_PIPE HAVE_PIPE
#endif

#endif /* MACOS */

#ifndef ioctl_t
#	define ioctl_t				int
#endif

#ifndef AC_SOCKET_INVALID
#	define AC_SOCKET_INVALID	(-1)
#endif
#ifndef AC_SOCKET_ERROR
#	define AC_SOCKET_ERROR		(-1)
#endif

#if !defined( HAVE_INET_ATON ) && !defined( inet_aton )
#	define inet_aton ldap_pvt_inet_aton
struct in_addr;
LDAP_F (int) ldap_pvt_inet_aton LDAP_P(( const char *, struct in_addr * ));
#endif

#if	defined(__WIN32) && defined(_ALPHA)
/* NT on Alpha is hosed. */
#	define AC_HTONL( l ) \
        ((((l)&0xffU)<<24) + (((l)&0xff00U)<<8) + \
         (((l)&0xff0000U)>>8) + (((l)&0xff000000U)>>24))
#	define AC_NTOHL(l) AC_HTONL(l)

#else
#	define AC_HTONL( l ) htonl( l )
#	define AC_NTOHL( l ) ntohl( l )
#endif

/* htons()/ntohs() may be broken much like htonl()/ntohl() */
#define AC_HTONS( s ) htons( s )
#define AC_NTOHS( s ) ntohs( s )

#ifdef LDAP_PF_LOCAL
#  if !defined( AF_LOCAL ) && defined( AF_UNIX )
#    define AF_LOCAL	AF_UNIX
#  endif
#  if !defined( PF_LOCAL ) && defined( PF_UNIX )
#    define PF_LOCAL	PF_UNIX
#  endif
#endif

#ifndef INET_ADDRSTRLEN
#	define INET_ADDRSTRLEN 16
#endif
#ifndef INET6_ADDRSTRLEN
#	define INET6_ADDRSTRLEN 46
#endif

#ifdef HAVE_GETADDRINFO
#	ifdef HAVE_GAI_STRERROR
#		define AC_GAI_STRERROR(x)	(gai_strerror((x)))
#	else
#		define AC_GAI_STRERROR(x)	(ldap_pvt_gai_strerror((x)))
		LDAP_F (char *) ldap_pvt_gai_strerror( int );
#	endif
#endif

#ifndef HAVE_GETPEEREID
LDAP_LUTIL_F( int ) getpeereid( int s, uid_t *, gid_t * );
#endif

/* DNS RFC defines max host name as 255. New systems seem to use 1024 */
#ifndef NI_MAXHOST
#define	NI_MAXHOST	256
#endif

#endif /* _AC_SOCKET_H_ */
