# specify nodist because we want these files to be regenerated 
# (so we pick up changes to package_version, sysconfdir, etc.)
nodist_man5_MANS = \
	s3fuse.1 \
	s3fuse.conf.5 \
	s3fuse_gs_get_token.1 \
	s3fuse_sha256_sum.1 \
	s3fuse_vol_key.1

PDF_MANS = $(nodist_man5_MANS)
PDFS = $(PDF_MANS:%=%.pdf)

EXTRA_DIST = \
	s3fuse.1.in \
	s3fuse.conf.5.awk \
	s3fuse.conf.5.pp \
	s3fuse_gs_get_token.1.in \
	s3fuse_sha256_sum.1.in \
	s3fuse_vol_key.1.in

CLEANFILES = $(nodist_man5_MANS) $(PDFS)
PKG_NAME_UPPER := $(shell echo $(PACKAGE_NAME) | tr a-z A-Z)
TODAY := $(shell date "+%Y-%m-%d")

%.1: %.1.in
	sed \
		-e "s/__PACKAGE_NAME__/$(PACKAGE_NAME)/g" \
		-e "s/__PACKAGE_NAME_UPPER__/$(PKG_NAME_UPPER)/g" \
		-e "s/__PACKAGE_VERSION__/$(PACKAGE_VERSION)/g" \
		-e "s/__TODAY__/$(TODAY)/g" \
		< $^ \
		> $@

s3fuse.conf.5: s3fuse.conf.5.awk ../src/base/s3fuse.conf.pp ../src/base/config.inc
	$(CC) $(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) -E -x c++ ../src/base/s3fuse.conf.pp | \
		sed -e 's/^ *//' -e 's/"//g' -e 's/;$$//' | \
		awk \
			-v "SYSCONFDIR=$(sysconfdir)" \
			-v "PACKAGE_NAME=$(PACKAGE_NAME)" \
			-v "PACKAGE_VERSION=$(PACKAGE_VERSION)" \
			-v "TODAY=$(TODAY)" \
			-F '%%%%' \
			-f s3fuse.conf.5.awk \
			> $@

%.pdf: %
	groff -mandoc $^ -t | pstopdf -i -o $@

pdfs: $(PDFS)
