/* The source in this file is derived from the reference implementation 
 * in RFC 2617. 
 * RFC 2617 is Copyright (C) The Internet Society (1999).  All Rights Reserved.
 *
 * The following copyright and licence statement covers all changes made to the 
 * reference implementation.
 * 
 * Key changes were: alteration to a plain C layout.
 * Create CvtBin function
 * Allow CalcHA1 to make use of precaculated username:password:realm hash's
 * to prevent squid knowing the users password (idea suggested in RFC 2617).
 */


/*
 * $Id: rfc2617.c,v 1.5.2.1 2003/09/23 16:05:41 hno Exp $
 *
 * DEBUG:
 * AUTHOR: RFC 2617 & Robert Collins
 *
 * SQUID Internet Object Cache  http://squid.nlanr.net/Squid/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from the
 *  Internet community.  Development is led by Duane Wessels of the
 *  National Laboratory for Applied Network Research and funded by the
 *  National Science Foundation.  Squid is Copyrighted (C) 1998 by
 *  the Regents of the University of California.  Please see the
 *  COPYRIGHT file for full details.  Squid incorporates software
 *  developed and/or copyrighted by other sources.  Please see the
 *  CREDITS file for full details.
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

#include "config.h"
#include <string.h>
#include "rfc2617.h"
#include "md5.h"

void
CvtHex(const HASH Bin, HASHHEX Hex)
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
	j = (Bin[i] >> 4) & 0xf;
	if (j <= 9)
	    Hex[i * 2] = (j + '0');
	else
	    Hex[i * 2] = (j + 'a' - 10);
	j = Bin[i] & 0xf;
	if (j <= 9)
	    Hex[i * 2 + 1] = (j + '0');
	else
	    Hex[i * 2 + 1] = (j + 'a' - 10);
    }
    Hex[HASHHEXLEN] = '\0';
}

void
CvtBin(const HASHHEX Hex, HASH Bin)
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHHEXLEN; i++) {
	unsigned char n;
	j = Hex[i];
	if (('0' <= j) && (j <= '9'))
	    n = j - '0';
	else
	    n = j - 'a' + 10;
	if (i % 2 == 0)
	    Bin[i / 2] = n << 4;
	else
	    Bin[i / 2] |= n;
    }
    Bin[HASHLEN] = '\0';
}


/* calculate H(A1) as per spec */
void
DigestCalcHA1(
    const char *pszAlg,
    const char *pszUserName,
    const char *pszRealm,
    const char *pszPassword,
    const char *pszNonce,
    const char *pszCNonce,
    HASH HA1,
    HASHHEX SessionKey
)
{
    MD5_CTX Md5Ctx;

    if (pszUserName) {
	MD5Init(&Md5Ctx);
	MD5Update(&Md5Ctx, pszUserName, strlen(pszUserName));
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, pszRealm, strlen(pszRealm));
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, pszPassword, strlen(pszPassword));
	MD5Final((unsigned char *)HA1, &Md5Ctx);
    }
    if (strcasecmp(pszAlg, "md5-sess") == 0) {
	MD5Init(&Md5Ctx);
	MD5Update(&Md5Ctx, HA1, HASHLEN);
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, pszNonce, strlen(pszNonce));
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, pszCNonce, strlen(pszCNonce));
	MD5Final((unsigned char *)HA1, &Md5Ctx);
    }
    CvtHex(HA1, SessionKey);
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
void
DigestCalcResponse(
    const HASHHEX HA1,		/* H(A1) */
    const char *pszNonce,	/* nonce from server */
    const char *pszNonceCount,	/* 8 hex digits */
    const char *pszCNonce,	/* client nonce */
    const char *pszQop,		/* qop-value: "", "auth", "auth-int" */
    const char *pszMethod,	/* method from the request */
    const char *pszDigestUri,	/* requested URL */
    const HASHHEX HEntity,	/* H(entity body) if qop="auth-int" */
    HASHHEX Response		/* request-digest or response-digest */
)
{
    MD5_CTX Md5Ctx;
    HASH HA2;
    HASH RespHash;
    HASHHEX HA2Hex;

    /*  calculate H(A2)
     */
    MD5Init(&Md5Ctx);
    MD5Update(&Md5Ctx, pszMethod, strlen(pszMethod));
    MD5Update(&Md5Ctx, ":", 1);
    MD5Update(&Md5Ctx, pszDigestUri, strlen(pszDigestUri));
    if (strcasecmp(pszQop, "auth-int") == 0) {
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, HEntity, HASHHEXLEN);
    }
    MD5Final((unsigned char *)HA2, &Md5Ctx);
    CvtHex(HA2, HA2Hex);

    /* calculate response
     */
    MD5Init(&Md5Ctx);
    MD5Update(&Md5Ctx, HA1, HASHHEXLEN);
    MD5Update(&Md5Ctx, ":", 1);
    MD5Update(&Md5Ctx, pszNonce, strlen(pszNonce));
    MD5Update(&Md5Ctx, ":", 1);
    if (*pszQop) {
	MD5Update(&Md5Ctx, pszNonceCount, strlen(pszNonceCount));
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, pszCNonce, strlen(pszCNonce));
	MD5Update(&Md5Ctx, ":", 1);
	MD5Update(&Md5Ctx, pszQop, strlen(pszQop));
	MD5Update(&Md5Ctx, ":", 1);
    }
    MD5Update(&Md5Ctx, HA2Hex, HASHHEXLEN);
    MD5Final((unsigned char *)RespHash, &Md5Ctx);
    CvtHex(RespHash, Response);
}
