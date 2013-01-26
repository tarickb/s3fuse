ACLOCAL_AMFLAGS = -I .

SUBDIRS = man src

coverage-report:
	@grep 'ac_cs_config=.*code-coverage' config.status 2>&1 >/dev/null \
		|| ( echo "must pass --enable-code-coverage to configure" && false )
	lcov -z -d .
	make check
	lcov -c -d . -o s3fuse.info
	lcov -r s3fuse.info /opt/\* -o s3fuse.info
	lcov -r s3fuse.info /usr/\* -o s3fuse.info
	lcov -r s3fuse.info \*/tests/\* -o s3fuse.info
	genhtml -o coverage-report s3fuse.info

cppcheck:
	cd src; cppcheck --enable=all -q * 2>&1 | grep -v -e '^\[[^]]*/tests/[^]]*\]:' -e '^\[[^]]*Makefile*[^]]*\]:'

test-file-headers:
	@for F in `find src -type f ! -path \*.svn\* ! -path \*/tests/\* \( -name \*.cc -or -name \*.h \) | sort`; do \
		MODF="$${F##src/}"; \
		HT=`head -n 2 $$F | tail -n 1 | sed -e 's/^ \* //'`; \
		\
		[ "$$HT" != "$$MODF" ] && echo NO MATCH: $$F; \
	done; true # so make doesn't report an error

test-header-guards:
	@for F in `find src -type f ! -path \*.svn\* -name \*.h | sort`; do \
		MODF=`echo $$F | sed -e 's/^src\//S3_/' -e 's/\//_/g' -e 's/\./_/g' | tr 'a-z' 'A-Z'`; \
		\
		if ! grep "#ifndef $$MODF" $$F >/dev/null || ! grep "#define $$MODF" $$F >/dev/null; then \
			echo FAILED: $$F; \
		fi; \
	done; true

test-using-decls:
	@for FILE in `find . ! -path \*/.svn/\* \( -name \*.cc -or -name \*.h \)`; do \
	  for CLASS in `grep '^using ' $$FILE | grep -v '^using namespace' | sed -e 's/^using.*:://' -e 's/;//'`; do \
	    REF_COUNT=`grep "\<$$CLASS\>" $$FILE | grep -v "^#" | grep -v "\"[^\"]*\<$$CLASS\>[^\"]*\"" | wc -l`; \
			\
	    if [ $$REF_COUNT -eq 1 ]; then \
	      echo $$FILE: unnecessary using declaration for [$$CLASS].; \
    	fi; \
	  done; \
	done; true

.PHONY: coverage-report cppcheck test-file-headers test-header-guards test-using-decls
