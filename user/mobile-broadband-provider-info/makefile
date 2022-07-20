## ADD Telstra Mobile
## ADD Wildcards instead of unknown
## country codes to dialing prefix http://www.worldatlas.com/aatlas/ctycodes.htm

URL    = git://git.gnome.org/mobile-broadband-provider-info
VERSION = 54ebd15f16bbbb1c4d0bbe682ac8819a63800ea0
METHOD = git
CONFIGURE = autogen.sh
mobile-broadband-provider-info_CLEAN = my_clean

ifndef USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_COMPLETE
AUTOMAKE_ROMFS = my_romfs
endif

include $(ROOTDIR)/tools/automake.inc

COUNTRY_FILTER_y = 
COUNTRY_FILTER_$(CONFIG_USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_AU) += au
COUNTRY_FILTER_$(CONFIG_USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_US) += us
COUNTRY_FILTER_$(CONFIG_USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_CA) += ca

ifdef CONFIG_USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_PARTIAL
SPACE =
SPACE +=
FILTER = (egrep '^($(subst $(SPACE),|,$(COUNTRY_FILTER_y))),' ; true)
else
FILTER = cat
endif

my_clean:
	rm -f serviceproviders.txt

serviceproviders.txt: add-iccid-prefixes.awk build/mobile-broadband-provider-info/serviceproviders.xml country-dialing-codes.txt
	xsltproc serviceproviders.xsl \
		build/mobile-broadband-provider-info/serviceproviders.xml | $(FILTER) \
		| awk -f add-iccid-prefixes.awk \
		> serviceproviders.txt
ifdef CONFIG_USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_UNKNOWN
	cp serviceproviders.txt serviceproviders.txt.orig
	awk -F , '{ print "*,*,"$$3","$$4"," }' < serviceproviders.txt.orig|\
		sort | uniq >> serviceproviders.txt
	rm -f serviceproviders.txt.orig
endif

ifndef CONFIG_USER_MOBILE_BROADBAND_PROVIDER_INFO_INSTALL_NONE
my_romfs: serviceproviders.txt
	$(ROMFSINST) /etc/serviceproviders.txt
else
my_romfs:
endif

