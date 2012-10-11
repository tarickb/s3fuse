SUBDIRS = base crypto objects services threads .

if BUILD_TESTS
	SUBDIRS += tests
endif

bin_PROGRAMS = s3fuse s3fuse_gs_get_token

# the order of archives in LDADD matters -- they must be listed such that if A
# requires symbols provided by B, then A must appear before B

s3fuse_SOURCES = main.cc
s3fuse_LDADD = \
	objects/libs3fuse_objects.a \
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
