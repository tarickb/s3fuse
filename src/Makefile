PUGIXML  := ~/packages/pugixml-1.0

LD       := $(CXX)

CPPFLAGS := -I $(PUGIXML)/include -D_FILE_OFFSET_BITS=64
CXXFLAGS := -g -Wall -Werror -O0
LDFLAGS  := -L $(PUGIXML)/lib -lcurl -lpugixml -lfuse -lpthread -lboost_thread

all: s3fuse

clean:
	rm -f *.o s3fuse

s3fuse: fs.o object.o openssl_locks.o request.o util.o thread_pool.o worker_thread.o main.o
	$(LD) $(LDFLAGS) -o $@ $^
