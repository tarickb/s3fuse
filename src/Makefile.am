if BUILD_TESTS
SUBDIRS = . tests
endif

bin_PROGRAMS = s3fuse s3fuse_gs_get_token

s3fuse_SOURCES = \
	async_handle.h \
	aws_service_impl.cc \
	aws_service_impl.h \
	config.cc \
	config.h \
	config.inc \
	directory.cc \
	directory.h \
	encrypted_file.cc \
	encrypted_file.h \
	file.cc \
	file.h \
	gs_service_impl.cc \
	gs_service_impl.h \
	logger.cc \
	logger.h \
	main.cc \
	object.cc \
	object.h \
	object_cache.cc \
	object_cache.h \
	request.cc \
	request.h \
	service.cc \
	service.h \
	service_impl.cc \
	service_impl.h \
	ssl_locks.cc \
	ssl_locks.h \
	symlink.cc \
	symlink.h \
	thread_pool.cc \
	thread_pool.h \
	util.cc \
	util.h \
	version.h \
	work_item.h \
	work_item_queue.h \
	worker_thread.cc \
	worker_thread.h \
	xattr.h \
	xattr_reference.cc \
	xattr_reference.h \
	xattr_value.cc \
	xattr_value.h \
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
	xattr.h \
	util.cc \
	util.h

dist_sysconf_DATA = \
	s3fuse.conf

CLEANFILES = \
	s3fuse.conf

s3fuse.conf: config.inc
	../build-config.sh $^ $@
