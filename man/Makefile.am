dist_man1_MANS = s3fuse.1
dist_man5_MANS = s3fuse.conf.5

EXTRA_DIST = s3fuse.conf.5.sh

CLEANFILES = s3fuse.conf.5

s3fuse.conf.5: s3fuse.conf.5.sh ../src/base/config.inc
	./$^ > $@
