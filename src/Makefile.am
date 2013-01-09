SUBDIRS = base crypto fs services threads .
DIST_SUBDIRS = $(SUBDIRS) tests

if BUILD_TESTS
SUBDIRS += tests
endif

bin_PROGRAMS = s3fuse s3fuse_gs_get_token s3fuse_sha256_sum

# the order of archives in LDADD matters -- they must be listed such that if A
# requires symbols provided by B, then A must appear before B

s3fuse_SOURCES = main.cc
s3fuse_LDADD = \
	fs/libs3fuse_fs.a \
	services/libs3fuse_services.a \
	crypto/libs3fuse_crypto.a \
	threads/libs3fuse_threads.a \
	base/libs3fuse_base.a \
	$(LDADD)

s3fuse_gs_get_token_SOURCES = gs_get_token.cc
s3fuse_gs_get_token_LDADD = \
	services/libs3fuse_services.a \
	crypto/libs3fuse_crypto.a \
	base/libs3fuse_base.a \
	$(LDADD)

s3fuse_sha256_sum_SOURCES = sha256_sum.cc
s3fuse_sha256_sum_LDADD = \
	crypto/libs3fuse_crypto.a \
	$(LDADD)
