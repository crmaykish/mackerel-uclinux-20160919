METHOD      = git
AUTORECONF  = ./boot.sh
URL         = https://github.com/openvswitch/ovs.git
VERSION     = 22d4614ddf83988a3771fb379ea029e663b4455a
CONFVARS    += ac_cv_search___atomic_load_8=no
CONFOPTS    += --with-linux=$(ROOTDIR)/$(LINUXDIR)
CONFOPTS    += --datarootdir="/usr/share"
ifdef CONFIG_USER_DPDK
CONFOPTS    += --with-dpdk=$(ROOTDIR)/user/dpdk/build/dpdk-2.2.0/build
endif
MAKEVARS    = 
ovs_ROMFS   = my_romfs

include $(ROOTDIR)/tools/automake.inc

my_romfs:
	$(MAKE) -C $(ROOTDIR)/$(LINUXDIR) \
		M=`pwd`/build/ovs/datapath/linux \
		modules_install \
		INSTALL_MOD_CMD="$(ROMFSINST) -S -r \"\"" \
		INSTALL_MOD_PATH=$(ROMFSDIR) \
		DEPMOD=$(ROOTDIR)/tools/depmod.sh
	$(ROMFSINST) -R /etc/openvswitch
	$(ROMFSINST) -s /var/run/openvswitch /etc/openvswitch

