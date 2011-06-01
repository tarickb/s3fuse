PUGIXML      = ~/packages/pugixml-1.0
bin_PROGRAMS = s3fuse xattr

AM_CPPFLAGS  = -I $(PUGIXML)/include -D_FILE_OFFSET_BITS=64
AM_CXXFLAGS  = -g -Wall -Werror -O0
AM_CFLAGS    = -g -Wall -Werror

s3fuse_LDADD     = -L $(PUGIXML)/lib -lpugixml
s3fuse_SOURCES = \
	config.cc \
	file_transfer.cc \
	fs.cc \
	logger.cc \
	main.cc \
	object_cache.cc \
	object.cc \
	open_file.cc \
	openssl_locks.cc \
	request.cc \
	thread_pool.cc \
	util.cc

xattr_SOURCES = \
	xattr.c
