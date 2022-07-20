
URL = ftp://ftp.ch.debian.org/mirror/debian-archive/debian/pool/main/s/sysklogd/sysklogd_1.5.orig.tar.gz
BUILDDIR = sysklogd-1.5
FINALTARGET = built
MAKEVARS = CC="$(CC)"
AUTOMAKE_ROMFS = my_romfs

include $(ROOTDIR)/tools/automake.inc

my_romfs:
	$(ROMFSINST) build/$(sysklogd_1.5.orig_BUILDDIR)/klogd /usr/sbin/klogd
	$(ROMFSINST) build/$(sysklogd_1.5.orig_BUILDDIR)/syslogd /usr/sbin/syslogd
