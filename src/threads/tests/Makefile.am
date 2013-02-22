TESTS = tests

noinst_PROGRAMS = tests

local_s3fuse_libs = $(addprefix ../../,$(s3fuse_libs))

tests_SOURCES = \
	async_handle.cc

tests_LDADD = $(local_s3fuse_libs) -lgtest -lgtest_main $(LDADD)
