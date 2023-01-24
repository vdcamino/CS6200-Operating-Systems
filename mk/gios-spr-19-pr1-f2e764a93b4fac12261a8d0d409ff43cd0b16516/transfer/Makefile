CC     = gcc
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
ASAN_LIBS = -static-libasan
CFLAGS := -Wall -Werror --std=gnu99 -g3

ARCH := $(shell uname)
ifneq ($(ARCH),Darwin)
  LDFLAGS += -lpthread
endif

# default is to build with address sanitizer enabled
all: transferclient transferserver

# the noasan version can be used with valgrind
all_noasan: transferclient_noasan transferserver_noasan

transferclient: transferclient.c
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)

transferserver: transferserver.c
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)


transferclient_noasan: transferclient_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

transferserver_noasan: transferserver_noasan.o
	$(CC) -o $@ $(CFLAGS)  $^ $(LDFLAGS)

%_noasan.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $<

.PHONY: clean

clean:
	rm -fr *.o transferclient transferserver transferclient_noasan transferserver_noasan
