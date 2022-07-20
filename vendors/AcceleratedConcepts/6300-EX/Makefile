#
# Makefile -- Build instructions for AcceleratedConcepts/6300-EX
#

ROMFSIMG  = $(IMAGEDIR)/rootfs.bin
UROMFSIMG = $(IMAGEDIR)/urootfs.bin
ZIMAGE    = $(IMAGEDIR)/zImage
IMAGE     = $(IMAGEDIR)/image.bin
UKERNEL   = $(IMAGEDIR)/ukernel.bin
IMAGESIZE = 7864320

ROMFS_DIRS = $(DEFAULT_ROMFS_DIRS)
ROMFS_DIRS += etc proc tmp usr/var var srv media sys mnt/config mnt/store /flash

DEVICES += $(DEVICE_PTY_64) \
	ledman,c,126,0

FLASH_DEVICES = \
	boot,c,90,0 \
	bootenv,c,90,2 \
	flash,c,90,4 \
	all,c,90,6 \
	image,c,90,8 \
	image1,c,90,10 \
	config,c,90,12 \
	configblock,b,31,6

TERMINFOS =  ansi linux screen vt100 vt102 vt200 vt220
TERMINFOS += xterm xterm-color xterm-xfree86

all:

clean: image.clean

romfs: romfs_dev romfs.dirs romfs.symlinks romfs.default romfs.rc romfs.skeleton romfs.version
	$(ROMFSINST) -p 555 /bin/flashsave
	$(ROMFSINST) -p 555 /sbin/hwclock
	$(ROMFSINST) -p 555 /etc/mkffs
	$(ROMFSINST) /etc/fw_env.config
	$(ROMFSINST) /etc/init.d/S10udev
	$(ROMFSINST) -s /bin/init /sbin/init
	$(ROMFSINST) -s /bin/ifconfig /sbin/ifconfig
	$(ROMFSINST) -s /sbin/setkey /usr/sbin/setkey
	$(ROMFSINST) -s /sbin/syslogd /usr/sbin/syslogd
	$(ROMFSINST) -s /sbin/klogd /usr/sbin/klogd
	$(ROMFSINST) -s /bin/haserl /usr/bin/haserl
	$(ROMFSINST) -s /var/run /run
	$(ROMFSINST) -s /sbin/dropbear /usr/sbin/dropbear
	$(ROMFSINST) -s /bin/dropbearkey /usr/bin/dropbearkey
	$(ROMFSINST) -s /bin/dropbearconvert /usr/bin/dropbearconvert
	echo "Accelerated Concepts 6300-EX" > $(ROMFSDIR)/etc/issue
	echo "6300-EX" > $(ROMFSDIR)/etc/hostname

romfs.post::
	$(ROMFSINST) /etc/fstab
	$(ROMFSINST) -R /bin/gettextize
	rm -rf $(ROMFSDIR)/share
	rm -rf $(ROMFSDIR)/etc/config
	rm -rf $(ROMFSDIR)/etc/default
	rm -f $(ROMFSDIR)/etc/usb.ids.gz
	rm -f $(ROMFSDIR)/lib/preloadable_libiconv.so
	-find $(ROMFSDIR)/usr/share/terminfo ! -type d | \
		egrep -v '(/$(shell echo $(strip $(TERMINFOS)) | sed 's/ /$$|\//g')$$)' | \
		xargs rm -f
	-find $(ROMFSDIR)/usr/share/terminfo -depth -type d | xargs rmdir 2> /dev/null
	rm -rf $(ROMFSDIR)/usr/share/man
	rm -rf $(ROMFSDIR)/lib/pkgconfig
	$(ROMFSINST) -R /bin/iconv
	$(ROMFSINST) -R /lib/pkgconfig
	$(ROMFSINST) -R /lib/gio
	$(ROMFSINST) -R /lib/dbus-1.0
	$(ROMFSINST) -e CONFIG_USER_BUSYBOX_INIT -A "^ttyS0" -a "ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100 # GENERIC_SERIAL" /etc/inittab
	for i in $(ROMFSDIR)/lib/modules/*/*; do [ ! -f "$$i.bin" ] || rm -f "$$i"; done
	$(ROOTDIR)/tools/libclean.sh $(ROMFSDIR)

uimage.bin:
	cp $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/zImage $(ZIMAGE)
	cat $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/dts/kirkwood-6300-ex.dtb >> $(ZIMAGE)
	mkimage -A arm -O linux -T kernel -C none -a 0x2000000 -e 0x2000000 -n "Linux-3.x" -d $(ZIMAGE) $(UKERNEL)
	mkimage -A arm -O linux -T ramdisk -C none -a 0x3000000 -n "ramdisk" -d $(ROMFSIMG) $(UROMFSIMG)

image: image.configs image.dir image.arm.zimage image.squashfs uimage.bin image.ukernel.bin image.tag image.copy
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(UKERNEL)  /tftpboot/
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(UKERNEL)  /tftpboot/uImage
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(UROMFSIMG)  /tftpboot/
	[ -n "$(NO_BUILD_INTO_TFTPBOOT)" ] || cp $(UROMFSIMG)  /tftpboot/uInitrd

include $(ROOTDIR)/vendors/config/config.dev
include $(ROOTDIR)/vendors/AcceleratedConcepts/vendor.mak
