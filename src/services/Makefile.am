SUBDIRS = .
DIST_SUBDIRS = . aws fvs gs

if WITH_AWS
SUBDIRS += aws
endif

if WITH_FVS
SUBDIRS += fvs
endif

if WITH_GS
SUBDIRS += gs
endif

noinst_LIBRARIES = libs3fuse_services.a

libs3fuse_services_a_SOURCES = \
	file_transfer.cc \
	file_transfer.h \
	impl.cc \
	impl.h \
	service.cc \
	service.h
