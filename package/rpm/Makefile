name=s3fuse
version=0.16
top_dir=$(shell pwd | sed 's/ /\\ /g')

all: clean dist rpm

dist:
	cd ../../ && make dist

rpm: ../../$(name)-$(version).tar.gz
	mkdir -p $(top_dir)/RPMS 
	mkdir -p $(top_dir)/SRPMS
	mkdir -p $(top_dir)/SOURCES
	mkdir -p $(top_dir)/BUILD
	mkdir -p $(top_dir)/BUILDROOT
	mkdir -p $(top_dir)/SPECS
	mkdir -p $(top_dir)/INSTALL
	cp $< SOURCES/
	rpmbuild -ba --define "_topdir $(top_dir)" $(name).spec
	

clean:
	rm -rf $(top_dir)/SOURCES $(top_dir)/RPMS $(top_dir)/SRPMS $(top_dir)/BUILD $(top_dir)/BUILDROOT $(top_dir)/SPECS $(top_dir)/INSTALL $(top_dir)/TMP

.PHONY: rpm
