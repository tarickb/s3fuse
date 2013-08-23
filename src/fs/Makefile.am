SUBDIRS = .
DIST_SUBDIRS = $(SUBDIRS) tests

if BUILD_TESTS
SUBDIRS += tests
endif
	
noinst_LIBRARIES = libs3fuse_fs.a

libs3fuse_fs_a_SOURCES = \
	bucket_volume_key.cc \
	bucket_volume_key.h \
	callback_xattr.cc \
	callback_xattr.h \
	directory.cc \
	directory.h \
	encrypted_file.cc \
	encrypted_file.h \
	encryption.cc \
	encryption.h \
	file.cc \
	file.h \
	glacier.cc \
	glacier.h \
	list_reader.cc \
	list_reader.h \
	local_file_store.cc \
	local_file_store.h \
	metadata.cc \
	metadata.h \
	mime_types.cc \
	mime_types.h \
	object.cc \
	object.h \
	object_metadata_cache.cc \
	object_metadata_cache.h \
	special.cc \
	special.h \
	static_xattr.cc \
	static_xattr.h \
	symlink.cc \
	symlink.h \
	xattr.h
