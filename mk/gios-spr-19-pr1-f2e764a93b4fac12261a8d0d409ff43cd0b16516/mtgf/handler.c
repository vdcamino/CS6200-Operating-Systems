#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "gfserver.h"
#include "gfserver-student.h"
#include "content.h"
#include "steque.h"

#define BUFFER_SIZE 50419
typedef int gfstatus_t;

#define  GF_OK 200
#define  GF_FILE_NOT_FOUND 400
#define  GF_ERROR 500

// define the request context struct
typedef struct steque_node{
	gfcontext_t *ctx;
	const char *path;
	void* arg;
}steque_node;

// initialization, global
pthread_mutex_t mutex;
pthread_cond_t mutex_cv=PTHREAD_COND_INITIALIZER;
steque_t *work_queue;


void *threadFunc(void *threadid);


// initialize the queue and threads
void gfserver_set_pthreads(int nthreads){
	work_queue = (steque_t *)malloc(sizeof(steque_t));
	steque_init(work_queue);
	pthread_t tid[nthreads];
	int i, tNum[nthreads];
	if (pthread_mutex_init(&mutex, NULL) != 0){
			fprintf(stderr, "\n mutex init has failed\n");
			exit(1);
	}
	for(i=0; i<nthreads; i++){
		/* create the threads; may be any number, in general */
		tNum[i] = i;
		printf("============== Create Thread No. %d. =============\n", i);
		// create threads, call the thread function
		if(pthread_create(&tid[i], NULL, threadFunc, &tNum[i]) != 0) {
			fprintf(stderr, "Unable to create producer thread\n");
			exit(1);
		}

	}
}

//
//  The purpose of this function is to handle a get request
//
//  The ctx is the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
ssize_t gfs_handler(gfcontext_t *ctx, const char *path, void* arg){
	// enqueue the request context, including ctx, file path, and arg.
	steque_node* node;
	node = (steque_node*)malloc(sizeof(steque_node));
	node->ctx = ctx;
	node->path = path;
	node->arg = arg;
	// mutex lock
	if(pthread_mutex_lock(&mutex)!=0){
		printf("\n mutex lock has failed\n");
		exit(1);
	}
	// enqueue a request context received from client, including ctx, path and arg
	steque_enqueue(work_queue,node);
	// printf("==============Enqueue a request=============\n");
	// mutex unlock
	if(pthread_mutex_unlock(&mutex)!=0){
		printf("\n mutex unlock has failed\n");
		exit(1);
	}
	/* notify the worker thread*/
	pthread_cond_signal(&mutex_cv);

	return 0;
}

/* thread function, worker thread handles the request from client*/
void *threadFunc(void *pArg){
	//threadnum = *((int*)pArg);
	int fildes;
	struct stat finfo;
	ssize_t file_len, file_sent, data_read, data_sent;
	char buffer[BUFFER_SIZE];
	gfstatus_t status;
	steque_node* node_obj;

	/* loop forever*/
	while(1){
		// mutex lock
		if(pthread_mutex_lock(&mutex)!=0){
			printf("\n mutex lock has failed\n");
			exit(1);
		}
		// wait if queue is empty
		while(steque_isempty(work_queue)){
			pthread_cond_wait(&mutex_cv, &mutex);
		}
		// pop the request from the queue
			node_obj = steque_pop(work_queue);
		// mutex unlock
		if(pthread_mutex_unlock(&mutex)!=0){
			printf("\n mutex unlock has failed\n");
			exit(1);
		}

		/* notify other threads */
		pthread_cond_signal(&mutex_cv);

		// server sends file to the client
		if(node_obj!=NULL){
			fildes = content_get(node_obj->path);
			if(fildes == -1){
				status = GF_FILE_NOT_FOUND;
				if(gfs_sendheader(node_obj->ctx, status, 0)<0){
					fprintf(stderr, "ERROR writing socket\n");
					exit(1);
				}
				free(node_obj);
				node_obj = NULL;
				exit(1);
		  }else {
				status = GF_OK;

				/* use fstat instead of lseek to get the file size, since lseek will
				 * change the position in the file.
				 */
				fstat(fildes, &finfo);
				file_len = finfo.st_size;
				// file_len = lseek(fildes, 0, SEEK_END);
				// printf("========The file length is %zu data========\n",file_len);

				if(gfs_sendheader(node_obj->ctx, status, file_len)<0){
					fprintf(stderr, "ERROR writing socket\n");
					exit(1);
				}
				/* send header and content */
			  file_sent = 0;
				do{
					memset(buffer, '\0', BUFFER_SIZE);
					/* read data from the fildes and store it in buffer, with size BUFFER_SIZE*/
					data_read = pread(fildes, buffer, BUFFER_SIZE, file_sent);
					// printf("========The server read %zu data========\n",data_read);
					if(data_read<0) {
						fprintf(stderr, "ERROR reading file\n");
						exit(1);
					}
					/* send the data, return the number of sent data*/
					data_sent = gfs_send(node_obj->ctx, buffer, data_read);
					if(data_sent<0) {
						fprintf(stderr, "ERROR writing socket\n");
						exit(1);
					}
					if(data_sent!=data_read){
						fprintf(stderr, "ERROR sending data\n");
						exit(1);
					}
					file_sent += data_sent; // track how many data sent

				}while(file_len>file_sent);
				free(node_obj); //free the memory space
				node_obj = NULL;
			}
		}
	}
}

/* cleanup*/
void gfserver_cleanup(){
	steque_destroy(work_queue); // destroy the queue
}
