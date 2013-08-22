SUBDIRS = .
DIST_SUBDIRS = $(SUBDIRS) tests

if BUILD_TESTS
SUBDIRS += tests
endif

noinst_LIBRARIES = libs3fuse_base.a

libs3fuse_base_a_SOURCES = \
	config.cc \
	config.h \
	config.inc \
	curl_easy_handle.cc \
	curl_easy_handle.h \
	logger.cc \
	logger.h \
	lru_cache_map.h \
	paths.cc \
	paths.h \
	request.cc \
	request.h \
	request_hook.h \
	s3fuse.conf.awk \
	s3fuse.conf.pp \
	static_list.h \
	statistics.cc \
	statistics.h \
	timer.h \
	xml.cc \
	xml.h

dist_sysconf_DATA = s3fuse.conf

CLEANFILES = s3fuse.conf

s3fuse.conf: s3fuse.conf.awk s3fuse.conf.pp config.inc
	$(CC) $(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) -E -x c++ s3fuse.conf.pp | \
		sed -e 's/^ *//' -e 's/"//g' -e 's/;$$//' | \
		awk \
			-v "PACKAGE_NAME=$(PACKAGE_NAME)" \
			-v "PACKAGE_VERSION=$(PACKAGE_VERSION)" \
			-F '%%%%' \
			-f s3fuse.conf.awk \
			> $@
