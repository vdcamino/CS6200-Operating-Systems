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

// request data structure
typedef struct steque_request{
	const char *filepath;
	void* arg;
    gfcontext_t *context;
} steque_request;

void* thread_handle_req(void* arg);

void cleanup_threads();

void set_pthreads(size_t nthreads);


#endif // __GF_SERVER_STUDENT_H__
