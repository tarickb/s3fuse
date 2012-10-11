noinst_LIBRARIES = libs3fuse_base.a

libs3fuse_base_a_SOURCES = \
	config.cc \
	config.h \
	config.inc \
	logger.cc \
	logger.h \
	request.cc \
	request.h \
	ssl_locks.cc \
	ssl_locks.h \
	timer.h \
	version.h \
	xml.cc \
	xml.h

dist_sysconf_DATA = s3fuse.conf

CLEANFILES = s3fuse.conf

s3fuse.conf: config.inc
	../../build-config.sh $^ $@
