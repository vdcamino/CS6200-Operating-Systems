CC     = gcc
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
ASAN_LIBS = -static-libasan
CFLAGS := -Wall -Werror --std=gnu99 -g3

ARCH := $(shell uname)
ifneq ($(ARCH),Darwin)
  LDFLAGS += -lpthread
endif

# default is to build with address sanitizer enabled
all: echoclient echoserver

# the noasan version can be used with valgrind
all_noasan: echoclient_noasan echoserver_noasan

echoclient: echoclient.c
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)

echoserver: echoserver.c
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)


echoclient_noasan: echoclient_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

echoserver_noasan: echoserver_noasan.o
	$(CC) -o $@ $(CFLAGS)  $^ $(LDFLAGS)

%_noasan.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $<

.PHONY: clean

clean:
	rm -fr *.o echoclient echoserver echoclient_noasan echoserver_noasan
