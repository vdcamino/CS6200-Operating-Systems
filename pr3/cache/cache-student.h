/*
 *  This file is for use by students to define anything they wish.  It is used by the proxy cache implementation
 */
 #ifndef __CACHE_STUDENT_H__
 #define __CACHE_STUDENT_H__

#include "steque.h"
#include "mqueue.h"
#include <sys/mman.h>
#include <semaphore.h>
#define MESSAGEQUEUE "/msgqueue"
#define PROXY_SEM "/proxysem"
#define CACHE_SEM "/cachesem"

// structure for command channel
typedef struct req_struct
{
    char path[MAX_REQUEST_LEN];
    char shm_id[10];
    size_t segsize;
} req_struct;

// structure for data channel
typedef struct shm_details
{
    sem_t proxysem;
    sem_t cachesem;
     sem_t filelen_sem;
    char shm_id[10];   
    size_t segsize;
    size_t filesize;
    int status;
    size_t read_len;  
    char data[];
    
}shm_details;

void * workerRoutine(void *arg);
void initSharedMemory(int segments,size_t segsize);
size_t processResponse(shm_details *shm, gfcontext_t *ctx);
void sendResponse(int fildes,char *memName,size_t segsize);
void queueMemory(shm_details *mem, char * memName);

 #endif // __CACHE_STUDENT_H__
