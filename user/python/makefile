
PYTHON_VERSION = 3.4.4
URL = http://www.python.org/ftp/python/$(PYTHON_VERSION)/Python-$(PYTHON_VERSION).tar.xz

CONFVARS += ac_cv_prog_READELF=$(CROSS_COMPILE)readelf
CONFVARS += ac_cv_buggy_getaddrinfo=no
CONFVARS += ac_cv_file__dev_ptmx=yes
CONFVARS += ac_cv_file__dev_ptc=no
CONFVARS += ac_cv_have_long_long_format=yes

Python-$(PYTHON_VERSION)_CONFOPTS += --prefix=/usr
Python-$(PYTHON_VERSION)_CONFOPTS += --without-ensurepip

Python-$(PYTHON_VERSION)_ROMFS = python_romfs

include $(ROOTDIR)/tools/automake.inc

python_romfs:
	$(ROMFSINST) -s python3.4 /usr/bin/python

