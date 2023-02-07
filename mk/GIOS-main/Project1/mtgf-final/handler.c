#include "gfserver-student.h"
#include "gfserver.h"
#include "content.h"
#include <pthread.h>
#include "steque.h"
#include "workload.h"

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
extern pthread_cond_t cond;


gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg){

	requestInfo *request= malloc(sizeof(requestInfo));
	request->ctx=(*ctx);
	request->filepath=path;
	steque_t *queue=(steque_t*)arg;
	pthread_mutex_lock(&mutex);
		steque_enqueue(queue,request);
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
	printf("Enqueued a request--->%s\n",path);
	(*ctx)=NULL;
	return gfh_success;
}