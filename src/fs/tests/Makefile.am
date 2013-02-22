noinst_PROGRAMS = \
	callback_xattr \
	decrypt_file \
	encrypt_file \
	static_xattr

local_s3fuse_libs = $(addprefix ../../,$(s3fuse_libs))

callback_xattr_SOURCES = callback_xattr.cc
callback_xattr_LDADD = $(local_s3fuse_libs) $(LDADD)

decrypt_file_SOURCES = decrypt_file.cc
decrypt_file_LDADD = $(local_s3fuse_libs) $(LDADD)

encrypt_file_SOURCES = encrypt_file.cc
encrypt_file_LDADD = $(local_s3fuse_libs) $(LDADD)

static_xattr_SOURCES = static_xattr.cc
static_xattr_LDADD = $(local_s3fuse_libs) $(LDADD)
