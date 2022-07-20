.EXPORT_ALL_VARIABLES:

ifdef CONFIG_USER_FLATFSD_ETC_CONFIG
CONFOPTS  = --with-ldap-conf-file=/etc/config/ldap.conf
CONFOPTS += --with-ldap-secret-file=/etc/config/ldap.secret
else
CONFOPTS  = --with-ldap-conf-file=/etc/ldap.conf
CONFOPTS += --with-ldap-secret-file=/etc/ldap.secret
endif

CFLAGS = -I$(ROOTDIR)/user/pam_ldap/build
CFLAGS += -I$(ROOTDIR)/user/pam_ldap
LDFLAGS += -static

all: build/build
	$(MAKE) -C build

build/build: makefile
	rm -rf build
	mkdir build
	(cd build; sh ../configure $(CONFIGURE_OPTS) $(CONFOPTS))
	touch build/build

clean:
	rm -rf build

romfs:
	$(ROMFSINST) build/pam_ldap.so /lib/security/pam_ldap.so
