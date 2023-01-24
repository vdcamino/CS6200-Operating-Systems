CC     = gcc
CFLAGS_WARN_OK = -Wall -Wno-unused-variable -I/usr/include/ImageMagick -g3
CFLAGS = $(CFLAGS_WARN_OK) -Werror 
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security

LIBS = -lpthread -lrt
ASAN_LIBS = -static-libasan

MAGICK_FLAGS = `pkg-config --cflags MagickCore`
MAGICK_LIBS = `pkg-config --libs MagickCore`

all: all_asan

all_noasan: magickminify_test_noasan minifyjpeg_main_noasan minifyjpeg_svc_noasan

all_asan: magickminify_test minifyjpeg_main minifyjpeg_svc


#### RPC Client Part ####
magickminify_noasan.o: magickminify.c
	$(CC) -o $@ -c $^ $(CFLAGS) $(MAGICK_FLAGS)

magickminify_test_noasan.o: magickminify_test.c
	$(CC) -o $@ -c $^ $(CFLAGS) $(MAGICK_FLAGS)

minify_via_rpc_noasan.o: minify_via_rpc.c minifyjpeg_xdr.c
	$(CC) -o $@ -c -o $@ $(CFLAGS_WARN_OK) minify_via_rpc.c

magickminify_test_noasan: magickminify_noasan.o magickminify_test_noasan.o
	$(CC) -o $@ $(CFLAGS) $(MAGICK_FLAGS) $^ $(LIBS) $(MAGICK_LIBS)

steque_noasan.o: steque.c
	$(CC) -c -o $@ $(CFLAGS) $^

minifyjpeg_svc_noasan.o: minifyjpeg_svc.c
	$(CC) -c -o $@ $(CFLAGS) $^

minifyjpeg_noasan.o: minifyjpeg.c
	$(CC) -c -o $@ $(CFLAGS) $^

minifyjpeg_main_noasan.o: minifyjpeg_main.c
	$(CC) -c -o $@ $(CFLAGS) $^

minifyjpeg_xdr_noasan.o: minifyjpeg_xdr.c
	$(CC) -o $@ -c $^ $(CFLAGS_WARN_OK) $(MAGICK_FLAGS)

minifyjpeg_main_noasan: minifyjpeg_main_noasan.o minify_via_rpc_noasan.o steque_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

minifyjpeg_svc_noasan: minifyjpeg_svc_noasan.o  minifyjpeg_noasan.o magickminify_noasan.o steque_noasan.o minifyjpeg_xdr_noasan.o
	$(CC) -o $@ $(CFLAGS) $(MAGICK_FLAGS) -DRPC_SVC_FG $^ $(LIBS) $(MAGICK_LIBS)

minifyjpeg_xdr.c:
	touch minifyjpeg_xdr.c

minifyjpeg.h:
	echo "You really want to run rpcgen" 
	touch minifyjpeg.h

magickminify.o: magickminify.c
	$(CC) -o $@ -c $^ $(CFLAGS) $(ASAN_FLAGS) $(MAGICK_FLAGS)

magickminify_test.o: magickminify_test.c
	$(CC) -o $@ -c $^ $(CFLAGS) $(ASAN_FLAGS) $(MAGICK_FLAGS)

magickminify_test: magickminify.o magickminify_test.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $(MAGICK_FLAGS) $^ $(LIBS) $(MAGICK_LIBS) $(ASAN_LIBS)

minify_via_rpc.o: minify_via_rpc.c
	$(CC) -c -o $@ $(CFLAGS_WARN_OK) $(ASAN_FLAGS) minify_via_rpc.c

minifyjpeg_main.o: minifyjpeg_main.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $^

minifyjpeg.o: minifyjpeg.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $^

minifyjpeg_svc.o: minifyjpeg_svc.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $^

minifyjpeg_xdr.o: minifyjpeg_xdr.c
	$(CC) -o $@ -c $^ $(CFLAGS_WARN_OK) $(ASAN_FLAGS) $(MAGICK_FLAGS)

minifyjpeg_main: minifyjpeg_main.o minify_via_rpc.o steque.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LIBS) $(ASAN_LIBS)

minifyjpeg_svc: minifyjpeg_svc.o  minifyjpeg.o magickminify.o steque.o minifyjpeg_xdr.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $(MAGICK_FLAGS) -DRPC_SVC_FG $^ $(LIBS) $(MAGICK_LIBS) $(ASAN_LIBS)

#### Cleanup ####
clean:
	rm -f *.o magickminify_test minifyjpeg_main minifyjpeg_svc magickminify_test_noasan minifyjpeg_main_noasan minifyjpeg_svc_noasan

