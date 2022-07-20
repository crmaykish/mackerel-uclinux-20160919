#URL = ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-8.36.tar.gz
URL = ftp://ftp.cs.stanford.edu/pub/exim/pcre/pcre-8.36.tar.gz
MAKEVARS =
pcre-8.36_CONFOPTS = --disable-cpp
ROMFS = pcre_cleanup
include $(ROOTDIR)/tools/automake.inc

pcre_cleanup:
	@for i in build/pcre-*-install/bin/*; do \
		[ -f "$$i" ] || continue; \
		$(ROMFSINST) -R "/bin/`basename $$i`"; \
	done
