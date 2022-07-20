#
#	Makefile -- Build instructions for AcceleratedConcepts/6300-DX
#

ROMFSIMG  = $(IMAGEDIR)/rootfs.bin
ZROMFSIMG = $(IMAGEDIR)/rootfs.bin.lzma
UROMFSIMG = $(IMAGEDIR)/urootfs.bin
VMLINUX   = $(IMAGEDIR)/vmlinux
IMAGE     = $(IMAGEDIR)/image.bin
KERNEL    = $(IMAGEDIR)/kernel
LZKERNEL  = $(IMAGEDIR)/kernel.lzma
UKERNEL   = $(IMAGEDIR)/ukernel.bin
IMAGESIZE = 32000000                   # much bigger than our flash, fix later

ROMFS_DIRS  = $(DEFAULT_ROMFS_DIRS)
ROMFS_DIRS += /tmp /usr/var /var /srv /media /sys /mnt/config /mnt/store /flash

DX_DEVICES = \
	ledman,c,126,0 \
	dk0,c,63,0 \
	dk1,c,63,1 \
	caldata,c,90,4

#
# This will hopefully be the final flash layout. But the prototype boards
# don't have a uboot good enough to save the environment, so we can't make
# this work just yet.
#
FLASH_DEVICES = \
	boot,c,90,0 \
	bootenv,c,90,2 \
	art,c,90,4 \
	config,c,90,6 \
	configblock,b,31,3 \
	kernel,c,90,8 \
	rootfs,c,90,10 \
	image,c,90,12 \
	all,c,90,14 \
	store,c,90,16 \
	kernel1,c,90,18 \
	rootfs1,c,90,20 \
	image1,c,90,22

TERMINFOS =  ansi linux screen vt100 vt102 vt200 vt220
TERMINFOS += xterm xterm-color xterm-xfree86

clean: image.clean

romfs: romfs_dev romfs.dirs romfs.symlinks romfs.default romfs.rc romfs.skeleton romfs.version
	$(ROMFSINST) -p 555 /bin/flashsave
	$(ROMFSINST) -p 555 /sbin/hwclock
	$(ROMFSINST) -p 555 /etc/init.d/S01lcd
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
	echo "Accelerated Concepts 6300-DX" > $(ROMFSDIR)/etc/issue
	echo "6300-DX" > $(ROMFSDIR)/etc/hostname

romfs.post::
	$(ROMFSINST) /etc/fstab
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
	rm -r $(ROMFSDIR)/lib/pkgconfig
	$(ROMFSINST) -e CONFIG_USER_BUSYBOX_INIT -A "^ttyATH0" -a "ttyATH0::respawn:/sbin/getty -L ttyATH0 115200 vt100 # GENERIC_SERIAL" /etc/inittab

./lzma: Makefile
	make -C $(ROOTDIR)/user/lzma clean
	make -C $(ROOTDIR)/user/lzma CC="$(HOSTCC)"
	cp  $(ROOTDIR)/user/lzma/build/C/Util/Lzma/lzma .
	make -C $(ROOTDIR)/user/lzma clean

image: ./lzma image.configs image.dir image.mips.vmlinux image.squashfs image.bin image.size image.tag image.copy
	$(OBJCOPY) -O binary $(VMLINUX) $(KERNEL)
	./lzma e $(KERNEL) $(LZKERNEL)
	[ `stat -c %s $(LZKERNEL)` -lt 2097152 ] || { echo "Kernel too big."; exit 1; }
	#./lzma -e $(ROMFSIMG) $(ZROMFSIMG)
	#mkimage -A mips -O linux -T multi -C lzma -a 0x80060000 -e 0x80060000 -n "Accelerated Concepts Linux-3.x" -d $(LZKERNEL):$(ZROMFSIMG) $(UKERNEL)
	mkimage -A mips -O linux -T kernel -C lzma -a 0x80060000 -e 0x80060000 -n "Linux-3.x" -d $(LZKERNEL) $(UKERNEL)
	mkimage -A mips -O linux -T ramdisk -C none -a 0x80800000 -n "ramdisk" -d $(ROMFSIMG) $(UROMFSIMG)
	SIZE=`cat $(ROMFSIMG) | wc -c` ; \
	[ "$$SIZE" -le 13238272 ] || { echo "ROOTFS Too big.";exit 1; }; \
	SIZE=`cat $(UKERNEL) | wc -c` ; \
	PAD=`expr 65536 - $$SIZE % 65536` ; \
	dd if=/dev/zero of=$(UKERNEL).pad bs=1 count=$$PAD
	cat $(UKERNEL) $(UKERNEL).pad $(ROMFSIMG) > $(IMAGE)
	printf '\0%s\0%s\0%s' $(VERSIONPKG) $(HW_VENDOR) $(HW_PRODUCT) >>$(IMAGE)
	$(ROOTDIR)/tools/cksum -b -o 2 $(IMAGE) >> $(IMAGE)
	rm -f $(UKERNEL).pad
	if [ -d /tftpboot ]; then \
		cp $(UKERNEL) /tftpboot ; \
		cp $(UROMFSIMG) /tftpboot ; \
		cp $(IMAGE) /tftpboot ; \
	fi

include $(ROOTDIR)/vendors/config/config.dev
# only our flash devices are really needed here,  udev does the rest
DEVICES := $(DX_DEVICES)
DEVICES_UDEV :=
DEVICES_SYSTEM :=
include $(ROOTDIR)/vendors/AcceleratedConcepts/vendor.mak
