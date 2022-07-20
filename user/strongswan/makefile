
VERSION = 5.2.2
URL = http://download.strongswan.org/strongswan-$(VERSION).tar.bz2
strongswan-$(VERSION)_ROMFS = strongswan_romfs

CONFOPTS :=
ifdef CONFIG_LIB_LIBSSL
CONFOPTS += --enable-openssl
endif
ifdef CONFIG_LIB_LIBUNBOUND
CONFOPTS += --enable-unbound
endif
ifdef CONFIG_LIB_LIBCURL
CONFOPTS += --enable-curl
endif

MAKEVARS = 

include $(ROOTDIR)/tools/automake.inc

.PHONY: strongswan_romfs
strongswan_romfs:
	$(ROMFSINST) -R /etc/ipsec.conf
	$(ROMFSINST) -R /etc/ipsec.d
	$(ROMFSINST) -R /etc/ipsec.secrets
	$(ROMFSINST) -s /var/run/ipsec.conf /etc/ipsec.conf
	$(ROMFSINST) -s /var/run/ipsec.secrets /etc/ipsec.secrets
	$(ROMFSINST) -s /var/run/ipsec.d /etc/ipsec.d
	sed -i 's/^\(charon.*\)$$/\1\n\ti_dont_care_about_security_and_use_aggressive_mode_psk = yes\n/' $(ROMFSDIR)/etc/strongswan.conf

