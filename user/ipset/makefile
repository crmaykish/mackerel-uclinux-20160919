PKG = ipset-6.23
URL = http://ipset.netfilter.org/$(PKG).tar.bz2
$(PKG)_BUILD = all modules
$(PKG)_MODVARS = INSTALL_MOD_CMD="$(ROMFSINST) -S -r \"\"" INSTALL_MOD_PATH=$(ROMFSDIR) DEPMOD=$(ROOTDIR)/tools/depmod.sh
$(PKG)_ROMFS = myromfs
AUTORECONF = autoreconf
MAKEVARS =
UCFRONT_ENV = 

CONFOPTS += --with-kbuild=$(ROOTDIR)/$(LINUXDIR)
CONFOPTS += --with-maxsets=$(CONFIG_IP_SET_MAX)

include $(ROOTDIR)/tools/automake.inc

myromfs:
	$($(PKG)_MODVARS) $(MAKE) -C build/$($(PKG)_BUILDDIR) modules_install
