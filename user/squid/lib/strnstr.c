/*
 *  Copyright (C) 2003 Nikos Mavroyanopoulos
 *
 *  This file is part of GNUTLS.
 *
 *  The GNUTLS library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public   
 *  License as published by the Free Software Foundation; either 
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

 /*
  * DW 2003/10/17:
  * Changed 'ssize_t' types to 'size_t'
  */

#include "config.h"
#ifndef HAVE_STRNSTR
#include <string.h>
#include <util.h>

char *strnstr(const char *haystack, const char *needle, size_t haystacklen)
{
        char *p;
        size_t plen;
        size_t len = strlen(needle);

        if (*needle == '\0')    /* everything matches empty string */
                return (char*) haystack;

        plen = haystacklen;
        for (p = (char*) haystack; p != NULL; p = memchr(p + 1, *needle, plen-1)) {
                plen = haystacklen - (p - haystack);

                if (plen < len) return NULL;

                if (strncmp(p, needle, len) == 0)
                        return (p);
        }
        return NULL;
}
#endif
