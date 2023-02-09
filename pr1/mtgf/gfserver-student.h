/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#include "gf-student.h"
#include "gfserver.h"
#include "content.h"
#include "steque.h"
#include <pthread.h>

void cleanup_threads();
void init_threads(size_t numthreads);

// request data structure
typedef struct steque_request{
	const char *filepath;
	void* arg;
    gfcontext_t *context;
} steque_request;

void* handleRequest(void*arg);

#endif // __GF_SERVER_STUDENT_H__
