#! /bin/sh
# $OpenLDAP: pkg/ldap/build/version.sh,v 1.5.2.39 2003/03/29 15:45:42 kurt Exp $
## Copyright 2000-2003 The OpenLDAP Foundation
## COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
## of this package for details.
#
DIR=`dirname $0`
. $DIR/version.var

if test $ol_patch != X ; then
	ol_version=${ol_major}.${ol_minor}.${ol_patch}
	ol_type=Release
elif test $ol_minor != X ; then
	ol_version=${ol_major}.${ol_minor}.${ol_patch}
	ol_type=Engineering
else
	ol_version=${ol_major}.${ol_minor}
	ol_type=Devel
	ol_api_lib=0:0:0
fi

ol_string="${ol_package} ${ol_version}-${ol_type}"

echo OL_PACKAGE=\"${ol_package}\"
echo OL_MAJOR=$ol_major
echo OL_MINOR=$ol_minor
echo OL_PATCH=$ol_patch
echo OL_API_INC=$ol_api_inc
echo OL_API_LIB=$ol_api_lib
echo OL_VERSION=$ol_version
echo OL_TYPE=$ol_type
echo OL_STRING=\"${ol_string}\"
echo OL_RELEASE_DATE=\"${ol_release_date}\"
