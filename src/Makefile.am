if BUILD_TESTS
SUBDIRS = crypto objects services threads . tests
endif

bin_PROGRAMS = s3fuse s3fuse_gs_get_token
noinst_LIBRARIES = libs3fuse_base.a libs3fuse_request.a

libs3fuse_request_a_SOURCES = \
	request.cc \
	request.h

libs3fuse_base_a_SOURCES = \
	config.cc \
	config.h \
	config.inc \
	logger.cc \
	logger.h \
	ssl_locks.cc \
	ssl_locks.h \
	version.h \
	xml.cc \
	xml.h

# the order of archives in LDADD matters -- they must be listed such that if A
# requires symbols provided by B, then A must appear before B

s3fuse_SOURCES = main.cc
s3fuse_LDADD = \
	objects/libs3fuse_objects.a \
	libs3fuse_request.a \
	services/libs3fuse_services.a \
	crypto/libs3fuse_crypto.a \
	threads/libs3fuse_threads.a \
	libs3fuse_base.a \
	$(LDADD)

s3fuse_gs_get_token_SOURCES = gs_get_token.cc
s3fuse_gs_get_token_LDADD = \
	libs3fuse_request.a \
	services/libs3fuse_services.a \
	libs3fuse_request.a \
	crypto/libs3fuse_crypto.a \
	libs3fuse_base.a \
	$(LDADD)

dist_sysconf_DATA = \
	s3fuse.conf

CLEANFILES = \
	s3fuse.conf

s3fuse.conf: config.inc
	../build-config.sh $^ $@
