bin_PROGRAMS = s3fuse xattr s3fuse_gs_get_token

AM_CXXFLAGS  = -g -Wall -Werror
AM_CFLAGS    = -g -Wall -Werror
AM_CPPFLAGS  = $(DEPS_CFLAGS) -D_FILE_OFFSET_BITS=64

s3fuse_LDFLAGS   = -Wl,--as-needed
s3fuse_LDADD     = $(DEPS_LIBS)
s3fuse_SOURCES = \
	aws_service_impl.cc \
	aws_service_impl.h \
	config.cc \
	config.inc \
	config.h \
	file_transfer.cc \
	file_transfer.h \
	fs.cc \
	fs.h \
	gs_service_impl.cc \
	gs_service_impl.h \
	logger.cc \
	logger.h \
	main.cc \
	mutexes.h \
	object_cache.cc \
	object_cache.h \
	object.cc \
	object.h \
	open_file.cc \
	open_file.h \
	openssl_locks.cc \
	openssl_locks.h \
	request.cc \
	request.h \
	service.cc \
	service.h \
	service_impl.h \
	thread_pool.cc \
	thread_pool.h \
	util.cc \
	util.h \
	version.h \
	xml.cc \
	xml.h

xattr_SOURCES = \
	xattr.c

s3fuse_gs_get_token_SOURCES = \
	aws_service_impl.cc \
	aws_service_impl.h \
	config.cc \
	config.inc \
	config.h \
	gs_service_impl.cc \
	gs_service_impl.h \
	gs_get_token.cc \
	logger.cc \
	logger.h \
	mutexes.h \
	object_cache.cc \
	object_cache.h \
	object.cc \
	object.h \
	openssl_locks.cc \
	openssl_locks.h \
	request.cc \
	request.h \
	service.cc \
	service.h \
	service_impl.h \
	util.cc \
	util.h

dist_sysconf_DATA = \
	s3fuse.conf

s3fuse.conf: config.inc
	../build-config.sh $^ $@
