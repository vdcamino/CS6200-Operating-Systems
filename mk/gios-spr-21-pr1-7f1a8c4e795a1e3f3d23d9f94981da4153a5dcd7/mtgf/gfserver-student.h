/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#define MAX_FILE_NAME_LEN 128

#include "gf-student.h"
#include "gfserver.h"
#include "content.h"

typedef struct queue_item {
  gfcontext_t *ctx;
  char path[MAX_FILE_NAME_LEN];
} queue_item;


void init_threads(size_t numthreads);
void cleanup_threads();

#endif // __GF_SERVER_STUDENT_H__