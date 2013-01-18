ACLOCAL_AMFLAGS = -I .

SUBDIRS = src
DIST_SUBDIRS = debian dist src

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

deb: dist
	rm -rf $(PACKAGE_NAME)-$(PACKAGE_VERSION)
	tar xfz $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
	cd $(PACKAGE_NAME)-$(PACKAGE_VERSION)/debian; debuild -us -uc
	rm -rf $(PACKAGE_NAME)-$(PACKAGE_VERSION)

rpm: dist
	rm -rf rpm-build
	mkdir -p rpm-build/SOURCES rpm-build/BUILD rpm-build/RPMS rpm-build/SRPMS rpm-build/SPECS
	cp $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz rpm-build/SOURCES
	rpmbuild \
		--define "_topdir $(PWD)/rpm-build" \
		--define "version $(VERSION)" \
		--define "release 1" \
		-ta rpm-build/SOURCES/*
	mv rpm-build/RPMS/*/* .
	rm -rf rpm-build

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

.PHONY: coverage-report rpm deb test-file-headers test-header-guards
