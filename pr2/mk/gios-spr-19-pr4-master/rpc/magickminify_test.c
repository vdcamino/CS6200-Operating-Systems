#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "magick/MagickCore.h"

extern void magickminify_init();
extern void* magickminify(void* src, ssize_t src_len, ssize_t* dst_len);
extern void magickminify_cleanup();


int main(int argc, char**argv){
  ImageInfo image_info;
  ExceptionInfo *exception;
  Image *image, *resize;
  void *src, *dst;
  ssize_t src_len, dst_len;
  size_t len;

  /*
    Initialize the image info structure and read an image.
  */
  magickminify_init();

  GetImageInfo(&image_info);
  exception = AcquireExceptionInfo();

  /* Read in a file */
  (void) strcpy(image_info.filename,argv[1]);

  image = ReadImage(&image_info, exception);

  if (exception->severity != UndefinedException)
    CatchException(exception);
  if (image == (Image *) NULL)
    exit(1);

  /* Convert to blob and minify */
  src = ImageToBlob(&image_info, image, &len, exception);
  src_len = len;

  dst = magickminify(src, src_len, &dst_len);

  resize = BlobToImage(&image_info, dst, dst_len, exception);
  (void) strcpy(resize->filename, argv[2]);  

  WriteImage(&image_info, resize);

  DestroyImage(image);
  DestroyImage(resize);
  DestroyExceptionInfo(exception);

  magickminify_cleanup();

  return(0);
}
