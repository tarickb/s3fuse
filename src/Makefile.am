PUGIXML      = ~/packages/pugixml-1.0
bin_PROGRAMS = s3fuse xattr

AM_CXXFLAGS  = -g -Wall -Werror
AM_CFLAGS    = -g -Wall -Werror

s3fuse_CPPFLAGS  = -I $(PUGIXML)/include -D_FILE_OFFSET_BITS=64
s3fuse_LDADD     = -L $(PUGIXML)/lib -lpugixml
s3fuse_SOURCES = \
	config.cc \
	config.inc \
	config.h \
	file_transfer.cc \
	file_transfer.h \
	fs.cc \
	fs.h \
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
	thread_pool.cc \
	thread_pool.h \
	util.cc \
	util.h \
	version.h

xattr_SOURCES = \
	xattr.c

dist_sysconf_DATA = \
	s3fuse.conf
