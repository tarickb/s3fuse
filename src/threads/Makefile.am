SUBDIRS = .
DIST_SUBDIRS = $(SUBDIRS) tests

if BUILD_TESTS
SUBDIRS += tests
endif

noinst_LTLIBRARIES = libs3fuse_threads.a

libs3fuse_threads_a_SOURCES = \
	async_handle.h \
	parallel_work_queue.h \
	pool.cc \
	pool.h \
	request_worker.cc \
	request_worker.h \
	work_item.h \
	work_item_queue.h \
	worker.cc \
	worker.h
