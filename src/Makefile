PUGIXML  := ~/packages/pugixml-1.0

LD       := $(CXX)

CPPFLAGS := -I $(PUGIXML)/include -D_FILE_OFFSET_BITS=64
CFLAGS   := -g -Wall -Werror
CXXFLAGS := -g -Wall -Werror -O0
LDFLAGS  := -L $(PUGIXML)/lib -lcurl -lpugixml -lfuse -lpthread -lboost_thread

DEP_DIR  := .deps
DEP_FILE  = $(DEP_DIR)/$(*F)
MAKE_DEP  = gcc -M $(CPPFLAGS) -o $(DEP_FILE).d $<

S3_SRCS  := config.cc \
						file_transfer.cc \
						fs.cc \
						object.cc \
						object_cache.cc \
						open_file.cc \
						openssl_locks.cc \
						request.cc \
						util.cc \
						thread_pool.cc \
						main.cc

%.o : %.cc
	@mkdir -p $(DEP_DIR);
	@$(MAKE_DEP); \
		cp $(DEP_FILE).d $(DEP_FILE).P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $(DEP_FILE).d >> $(DEP_FILE).P; \
		rm -f $(DEP_FILE).d
	$(COMPILE.cc) -o $@ $<

all: s3fuse xattr

-include $(S3_SRCS:%.cc=$(DEP_DIR)/%.P)

clean:
	rm -f *.o s3fuse xattr

depclean: clean
	rm -f $(DEP_DIR)/*.P

s3fuse: $(S3_SRCS:.cc=.o)
	$(LD) $(LDFLAGS) -o $@ $^

xattr: xattr.o
	$(LD) -o $@ $^
