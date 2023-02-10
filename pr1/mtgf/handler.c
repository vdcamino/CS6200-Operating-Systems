#include "gfserver-student.h"
#include "workload.h"
#include "gfserver.h"
#include "content.h"
#include <stdlib.h>

//
//  The purpose of this function is to handle a get request
//
//  The ctx is a pointer to the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//

extern pthread_mutex_t mutex;
extern steque_t* work_queue;
extern pthread_cond_t cond;

gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg){
	steque_request* req = (steque_request*)malloc(sizeof(*req));
	req->context = (*ctx);
	req->filepath = path;
	req->arg = arg;
	// mutex lock
	if(!(pthread_mutex_lock(&mutex) == 0))
		exit(56);
	// enqueue request
	steque_enqueue(work_queue,req);
	// mutex unlock
	if(!(pthread_mutex_unlock(&mutex) == 0))
		exit(38);
	pthread_cond_signal(&cond);
	(*ctx) = NULL; // fix error "handler did not ctx to NULL"
	return gfh_success;
}