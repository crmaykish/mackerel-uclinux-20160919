/* Generic alloca.h */
/* $OpenLDAP: pkg/ldap/include/ac/alloca.h,v 1.12.2.1 2003/03/03 17:10:03 kurt Exp $ */
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

#ifndef _AC_ALLOCA_H
#define _AC_ALLOCA_H

/*
 * use of alloca is disallowed as it is machine dependent
 */
#error  "alloca() not supported, use malloc()"

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
extern char *(alloca)();
#   endif
#  endif
# endif
#endif


#endif /* _AC_ALLOCA_H */
