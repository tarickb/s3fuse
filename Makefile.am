ACLOCAL_AMFLAGS = -I .

SUBDIRS = src
DIST_SUBDIRS = debian dist osx src

$(PACKAGE_NAME)-$(PACKAGE_VERSION)_osx.tar.gz: dist
	rm -rf osx-build
	mkdir -p osx-build/$(PACKAGE_NAME).app/Contents/MacOS
	mkdir -p osx-build/$(PACKAGE_NAME).app/Contents/Resources/Scripts
	mkdir -p osx-build/$(PACKAGE_NAME).app/Contents/Resources/libs
	mkdir -p osx-build/$(PACKAGE_NAME).app/Contents/Resources/bin
	cd osx-build; tar xfz ../$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
	cd osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION); \
		./configure --enable-darwin --enable-osx-bundle; \
		make
	sed \
		-e "s/__PACKAGE_VERSION__/$(PACKAGE_VERSION)/g" \
		-e "s/__PACKAGE_VERSION_WITH_REV__/$(PACKAGE_VERSION_WITH_REV)/g" \
		< osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/osx/Info.plist.in \
		> osx-build/$(PACKAGE_NAME).app/Contents/Info.plist
	cp \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/osx/applet \
		osx-build/$(PACKAGE_NAME).app/Contents/MacOS
	cp \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/COPYING \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/ChangeLog \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/README \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/osx/applet.icns \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/osx/applet.rsrc \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/src/base/s3fuse.conf \
		osx-build/$(PACKAGE_NAME).app/Contents/Resources
	cp \
		osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/osx/main.scpt \
		osx-build/$(PACKAGE_NAME).app/Contents/Resources/Scripts
	cd osx-build/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/src; \
		echo `grep bin_PROGRAMS Makefile.am` ../../$(PACKAGE_NAME).app/Contents/Resources/bin | \
			sed -e 's/^.*=//' | \
			tr ' ' '\n' | \
			xargs cp
	cd osx-build; \
		for F in ./$(PACKAGE_NAME).app/Contents/Resources/bin/*; do \
			dylibbundler -od -b -x $$F -d $(PACKAGE_NAME).app/Contents/Resources/libs/; \
		done; true
	cd osx-build; tar cfz ../$@ $(PACKAGE_NAME).app
	rm -rf osx-build

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

dmg: $(PACKAGE_NAME)-$(PACKAGE_VERSION)_osx.tar.gz
	rm -rf dmg-build
	mkdir dmg-build
	cd dmg-build; tar xfz ../$<
	cp \
		dmg-build/$(PACKAGE_NAME).app/Contents/Resources/COPYING \
		dmg-build/$(PACKAGE_NAME).app/Contents/Resources/ChangeLog \
		dmg-build/$(PACKAGE_NAME).app/Contents/Resources/README \
		dmg-build/
	hdiutil create \
		-fs HFSX -layout SPUD -format UDZO -scrub -uid 99 -gid 99 -ov \
		-volname "$(PACKAGE_NAME)-$(PACKAGE_VERSION)" \
		"$(PACKAGE_NAME)-$(PACKAGE_VERSION).dmg" \
		-srcfolder dmg-build
	rm -rf dmg-build

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

test-using-decls:
	@for FILE in `find . ! -path \*/.svn/\* \( -name \*.cc -or -name \*.h \)`; do \
	  for CLASS in `grep '^using ' $$FILE | sed -e 's/^using.*:://' -e 's/;//'`; do \
	    REF_COUNT=`grep "\<$$CLASS\>" $$FILE | wc -l`; \
			\
	    if [ $$REF_COUNT -eq 1 ]; then \
	      echo $$FILE: unnecessary using declaration for [$$CLASS].; \
    	fi; \
	  done; \
	done; true

.PHONY: coverage-report cppcheck deb dmg rpm test-file-headers test-header-guards test-using-decls
