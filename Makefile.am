ACLOCAL_AMFLAGS = -I .

SUBDIRS = src
DIST_SUBDIRS = debian dist src

coverage-report:
	lcov -z -d .
	make check
	lcov -c -d . -o s3fuse.info
	lcov -r s3fuse.info /opt/\* -o s3fuse.info
	lcov -r s3fuse.info /usr/\* -o s3fuse.info
	lcov -r s3fuse.info \*/tests/\* -o s3fuse.info
	genhtml -o coverage-report s3fuse.info

.PHONY: coverage-report
