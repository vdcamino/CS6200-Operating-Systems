 // This file is for use by students to define anything they wish.  It is used by the gf client implementation
#ifndef __GF_CLIENT_STUDENT_H__
#define __GF_CLIENT_STUDENT_H__

#include "workload.h"
#include "gfclient.h"
#include "gf-student.h"
#include <pthread.h>
#include "steque.h"

#define BUFSIZE 512

void *thread_handle_req();

void clear_buffer(char *buffer, int bufsize);

void set_request_main_code(char* filepath);

void set_pthreads(pthread_t* threads);


 #endif // __GF_CLIENT_STUDENT_H__