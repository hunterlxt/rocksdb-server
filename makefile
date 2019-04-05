PLATFORM=OS_LINUX
PLATFORM_LDFLAGS= -luv -lpthread -lrt -lsnappy -lgflags -lz -lbz2 -llz4 -lzstd
PLATFORM_CXXFLAGS= -std=c++11  -faligned-new -DHAVE_ALIGNED_NEW -DROCKSDB_PLATFORM_POSIX -DROCKSDB_LIB_IO_POSIX  -DOS_LINUX -fno-builtin-memcmp -DROCKSDB_FALLOCATE_PRESENT -DSNAPPY -DGFLAGS=1 -DZLIB -DBZIP2 -DLZ4 -DZSTD -DROCKSDB_MALLOC_USABLE_SIZE -DROCKSDB_PTHREAD_ADAPTIVE_MUTEX -DROCKSDB_BACKTRACE -DROCKSDB_RANGESYNC_PRESENT -DROCKSDB_SCHED_GETCPU_PRESENT -march=native  -DHAVE_SSE42 -DHAVE_PCLMUL -DROCKSDB_SUPPORT_THREAD_LOCAL

all: server client librocksdb

server: server.cc
	g++ server.cc -o run-server -std=c++11 lib/librocksdb.a -I. $(PLATFORM_LDFLAGS)

client: client-example.cc
	g++ client-example.cc -o run-client -std=c++11 lib/librocksdb.a -I. $(PLATFORM_LDFLAGS)

clean:
	rm -rf ./run-server ./run-client

librocksdb: lib/librocksdb.a
