noinst_LIBRARIES = libs3fuse_services.a
bin_PROGRAMS = s3fuse_gs_get_token

AM_CPPFLAGS += -I../

libs3fuse_services_a_SOURCES = \
	aws_impl.cc \
	aws_impl.h \
	gs_impl.cc \
	gs_impl.h \
	service.cc \
	service.h \
	impl.cc \
	impl.h

s3fuse_gs_get_token_SOURCES = gs_get_token.cc
s3fuse_gs_get_token_LDADD = libs3fuse_services.a $(LDADD)
