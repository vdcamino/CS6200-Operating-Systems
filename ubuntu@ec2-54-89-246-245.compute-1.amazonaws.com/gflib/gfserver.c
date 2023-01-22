#include "gfserver-student.h"

// Modify this file to implement the interface specified in
 // gfserver.h.

void gfs_abort(gfcontext_t **ctx){
}

gfserver_t* gfserver_create(){
    // not yet implemented
    return (gfserver_t *)NULL;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    // not yet implemented
    return -1;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    // not yet implemented
    return -1;
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){}

void gfserver_serve(gfserver_t **gfs){}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){}



