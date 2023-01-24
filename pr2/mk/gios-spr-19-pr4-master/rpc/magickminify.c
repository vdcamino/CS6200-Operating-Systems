#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "magick/MagickCore.h"

void magickminify_init(){
  MagickCoreGenesis("./", MagickTrue);
}

void* magickminify(void* src, ssize_t src_len, ssize_t* dst_len){
  Image *image, *resize;
  ImageInfo image_info;
  ExceptionInfo *exception;
  size_t len;
  void *ans;

  GetImageInfo(&image_info);
  exception = AcquireExceptionInfo();

  image = BlobToImage(&image_info, src, src_len, exception);

  if (exception->severity != UndefinedException)
    CatchException(exception);
  if (image == (Image *) NULL)
    exit(1);

  resize = MinifyImage(image, exception);

  if (exception->severity != UndefinedException)
    CatchException(exception);
  if (image == (Image *) NULL)
    exit(1);

  ans = ImageToBlob(&image_info, resize, &len, exception);
  
  if(dst_len != NULL)
    *dst_len = len;

  DestroyImage(image);
  DestroyImage(resize);
  DestroyExceptionInfo(exception);

  return ans;
}

void magickminify_cleanup(){
  MagickCoreTerminus();
}

