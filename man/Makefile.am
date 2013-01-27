dist_man1_MANS = \
	s3fuse.1 \
	s3fuse_gs_get_token.1 \
	s3fuse_sha256_sum.1 \
	s3fuse_vol_key.1

# specify nodist for this file because we want it to be regenerated 
# (otherwise it won't pick up on changes to sysconfdir)
nodist_man5_MANS = s3fuse.conf.5

PDF_MANS = $(dist_man1_MANS) $(nodist_man5_MANS)
PDFS = $(PDF_MANS:%=%.pdf)

EXTRA_DIST = s3fuse.conf.5.sh

CLEANFILES = $(nodist_man5_MANS) $(PDFS)

s3fuse.conf.5: s3fuse.conf.5.sh ../src/base/config.inc
	export SYSCONFDIR=$(sysconfdir); ./$^ > $@

%.pdf: %
	groff -mandoc $^ -t | pstopdf -i -o $@

pdfs: $(PDFS)
