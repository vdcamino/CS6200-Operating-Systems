#include "gfserver-student.h"
#include "gfserver.h"
#include "steque.h"
#include "content.h"
#include <pthread.h>

#include "workload.h"

// extern pthread_mutex_t steque_mutex;
// extern pthread_cond_t cv_server;
// extern steque_t steque_queue;

extern void enqueue_item(gfcontext_t ***ctx, const char *path);

//
//  The purpose of this function is to handle a get request
//
//  The ctx is a pointer to the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg){

	enqueue_item(&ctx, path);
    // pthread_mutex_lock(&steque_mutex); // LOCK

	// // Enqueue the CTX and Path from request onto the queue as queue_item
	// queue_item * item = (queue_item*)malloc(sizeof(queue_item));
	// item->ctx = *ctx;
	// memset(item->path, 0, MAX_FILE_NAME_LEN);
	// memcpy(item->path, path, strlen(path));
	// steque_enqueue(&steque_queue, item);

	// // COND SIGNAL TO WORKER CV
	// pthread_cond_signal(&cv_server);	

    // pthread_mutex_unlock(&steque_mutex); // UNLOCKaaa

	// free(*ctx);
	*ctx = NULL;

	return gfh_success;
}