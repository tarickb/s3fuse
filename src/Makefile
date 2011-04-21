PUGIXML  := ~/packages/pugixml-1.0

LD       := $(CXX)

CPPFLAGS := -I $(PUGIXML)/include -D_FILE_OFFSET_BITS=64
CXXFLAGS := -g -Wall -Werror
LDFLAGS  := -L $(PUGIXML)/lib -lcurl -lpugixml -lfuse -lpthread -lboost_thread

all: s3fuse

clean:
	rm -f *.o s3fuse

s3fuse: s3_async_queue.o s3_fs.o s3_util.o s3_request.o main.o
	$(LD) $(LDFLAGS) -o $@ $^
