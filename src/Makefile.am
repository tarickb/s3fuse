SUBDIRS = . tests

bin_PROGRAMS = s3fuse s3fuse_gs_get_token

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
	object_cache.cc \
	object_cache.h \
	object.cc \
	object.h \
	open_file.cc \
	open_file.h \
	request.cc \
	request.h \
	service.cc \
	service.h \
	service_impl.cc \
	service_impl.h \
	ssl_locks.cc \
	ssl_locks.h \
	thread_pool.cc \
	thread_pool.h \
	util.cc \
	util.h \
	version.h \
	xml.cc \
	xml.h

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
	object_cache.cc \
	object_cache.h \
	object.cc \
	object.h \
	request.cc \
	request.h \
	service.cc \
	service.h \
	service_impl.cc \
	service_impl.h \
	ssl_locks.cc \
	ssl_locks.h \
	util.cc \
	util.h

dist_sysconf_DATA = \
	s3fuse.conf

s3fuse.conf: config.inc
	../build-config.sh $^ $@
