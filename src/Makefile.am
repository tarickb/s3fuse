if BUILD_TESTS
SUBDIRS = crypto objects services threads . tests
endif

bin_PROGRAMS = s3fuse s3fuse_gs_get_token

s3fuse_SOURCES = \
	async_handle.h \
	aws_service_impl.cc \
	aws_service_impl.h \
	base64.cc \
	base64.h \
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
	hmac_sha1.cc \
	hmac_sha1.h \
	logger.cc \
	logger.h \
	main.cc \
	md5.cc \
	md5.h \
	object.cc \
	object.h \
	object_cache.cc \
	object_cache.h \
	request.cc \
	request.h \
	request_worker_thread.cc \
	request_worker_thread.h \
	service.cc \
	service.h \
	service_impl.cc \
	service_impl.h \
	sha256.cc \
	sha256.h \
	sha256_hash_list.h \
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

CLEANFILES = \
	s3fuse.conf

s3fuse.conf: config.inc
	../build-config.sh $^ $@
