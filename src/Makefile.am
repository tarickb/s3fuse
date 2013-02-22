SUBDIRS = base crypto fs services threads .
DIST_SUBDIRS = $(SUBDIRS) tests

if BUILD_TESTS
SUBDIRS += tests
endif

# the order of archives in s3fuse_libs matters -- they must be listed such that if A
# requires symbols provided by B, then A must appear before B

s3fuse_libs = \
	fs/libs3fuse_fs.a \
	services/aws/libs3fuse_services_aws.a \
	services/gs/libs3fuse_services_gs.a \
	threads/libs3fuse_threads.a \
	services/libs3fuse_services.a \
	crypto/libs3fuse_crypto.a \
	base/libs3fuse_base.a

bin_PROGRAMS = s3fuse s3fuse_gs_get_token s3fuse_sha256_sum s3fuse_vol_key

s3fuse_SOURCES = main.cc operations.cc operations.h
s3fuse_LDADD = $(s3fuse_libs) $(LDADD)

s3fuse_gs_get_token_SOURCES = gs_get_token.cc
s3fuse_gs_get_token_LDADD = $(s3fuse_libs) $(LDADD)

s3fuse_sha256_sum_SOURCES = sha256_sum.cc
s3fuse_sha256_sum_LDADD = $(s3fuse_libs) $(LDADD)

s3fuse_vol_key_SOURCES = vol_key.cc
s3fuse_vol_key_LDADD = $(s3fuse_libs) $(LDADD)
