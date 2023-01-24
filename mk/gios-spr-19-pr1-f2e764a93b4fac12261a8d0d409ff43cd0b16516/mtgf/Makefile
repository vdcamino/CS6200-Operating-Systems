CC     = gcc
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
ASAN_LIBS = -static-libasan
CFLAGS := -Wall -Werror --std=gnu99 -g3

ARCH := $(shell uname)
ifneq ($(ARCH),Darwin)
  LDFLAGS += -lpthread
endif

# default is to build with address sanitizer enabled
all: gfserver_main gfclient_download

# the noasan version can be used with valgrind
all_noasan: gfserver_main_noasan gfclient_download_noasan

gfserver_main: gfserver.o handler.o gfserver_main.o content.o steque.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $(CURL_CFLAGS) $^ $(LDFLAGS) $(CURL_LIBS) $(ASAN_LIBS)

gfclient_download: gfclient.o workload.o gfclient_download.o steque.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS)  $(ASAN_LIBS)

gfserver_main_noasan: gfserver_noasan.o handler_noasan.o gfserver_main_noasan.o content_noasan.o steque_noasan.o
	$(CC) -o $@ $(CFLAGS) $(CURL_CFLAGS) $^ $(LDFLAGS) $(CURL_LIBS)

gfclient_download_noasan: gfclient_noasan.o workload_noasan.o gfclient_download_noasan.o steque_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

%_noasan.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $<

.PHONY: clean

clean:
	mv gfserver.o gfserver.o.tmp
	mv gfserver_noasan.o gfserver_noasan.o.tmp
	mv gfclient_noasan.o gfclient_noasan.o.tmp
	mv gfclient.o gfclient.o.tmp
	rm -fr *.o gfserver_main gfclient_download gfserver_main_noasan gfclient_download_noasan
	mv gfserver.o.tmp gfserver.o
	mv gfserver_noasan.o.tmp gfserver_noasan.o
	mv gfclient_noasan.o.tmp gfclient_noasan.o
	mv gfclient.o.tmp gfclient.o

