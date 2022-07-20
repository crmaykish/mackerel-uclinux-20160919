/* Generic ctype.h */
/* $OpenLDAP: pkg/ldap/include/ac/ctype.h,v 1.10.2.1 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_CTYPE_H
#define _AC_CTYPE_H

#include <ctype.h>

#undef TOUPPER
#undef TOLOWER

#ifdef C_UPPER_LOWER
# define TOUPPER(c)	(islower(c) ? toupper(c) : (c))
# define TOLOWER(c)	(isupper(c) ? tolower(c) : (c))
#else
# define TOUPPER(c)	toupper(c)
# define TOLOWER(c)	tolower(c)
#endif

#endif /* _AC_CTYPE_H */
