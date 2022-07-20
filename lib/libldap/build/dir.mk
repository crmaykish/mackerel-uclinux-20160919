# $OpenLDAP: pkg/ldap/build/dir.mk,v 1.11.2.1 2003/03/03 17:10:01 kurt Exp $
## Copyright 1998-2003 The OpenLDAP Foundation
## COPYING RESTRICTIONS APPLY.  See COPYRIGHT File in top level directory
## of this package for details.
##---------------------------------------------------------------------------
##
## Makes subdirectories
##


all-common: FORCE
	@echo "Making all in `$(PWD)`"
	@for i in $(SUBDIRS) $(ALLDIRS); do 		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) all );		\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

install-common: FORCE
	@echo "Making install in `$(PWD)`"
	@for i in $(SUBDIRS) $(INSTALLDIRS); do 	\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) install );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

clean-common: FORCE
	@echo "Making clean in `$(PWD)`"
	@for i in $(SUBDIRS) $(CLEANDIRS); do		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) clean );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

veryclean-common: FORCE
	@echo "Making veryclean in `$(PWD)`"
	@for i in $(SUBDIRS) $(CLEANDIRS); do		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) veryclean );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

depend-common: FORCE
	@echo "Making depend in `$(PWD)`"
	@for i in $(SUBDIRS) $(DEPENDDIRS); do		\
		echo "  Entering subdirectory $$i";		\
		( cd $$i; $(MAKE) $(MFLAGS) depend );	\
		if test $$? != 0 ; then exit 1; fi ;	\
		echo " ";								\
	done

Makefile: $(top_srcdir)/build/dir.mk
