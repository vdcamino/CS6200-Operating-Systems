/*
 *  This file is for use by students to define anything they wish.  It is used by both proxy server implementation
 */
 #ifndef __SERVER_STUDENT_H__
 #define __SERVER_STUDENT_H__

 #include "steque.h"
 size_t write_cb( void *ptr, size_t size,size_t nmemb,void* userdata);
  typedef struct customdata
 {
     gfcontext_t *ctx;
    char * response;
   size_t filesize;
    void* status;
 }customdata;
 #endif // __SERVER_STUDENT_H__