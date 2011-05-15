PUGIXML  := ~/packages/pugixml-1.0

LD       := $(CXX)

CPPFLAGS := -I $(PUGIXML)/include -D_FILE_OFFSET_BITS=64
CFLAGS   := -g -Wall -Werror
CXXFLAGS := -g -Wall -Werror -O0
LDFLAGS  := -L $(PUGIXML)/lib -lcurl -lpugixml -lfuse -lpthread -lboost_thread

all: s3fuse xattr

clean:
	rm -f *.o s3fuse xattr

s3fuse: file_transfer.o fs.o object.o object_cache.o open_file.o openssl_locks.o request.o util.o thread_pool.o main.o
	$(LD) $(LDFLAGS) -o $@ $^

xattr: xattr.o
	$(LD) -o $@ $^
