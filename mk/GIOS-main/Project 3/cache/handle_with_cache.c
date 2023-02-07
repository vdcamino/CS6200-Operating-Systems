#include "gfserver.h"
#include "cache-student.h"

#define BUFSIZE (6201)
/*****************************/
steque_t shm_queue;
pthread_mutex_t shm_mutex =PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t shm_cond = PTHREAD_COND_INITIALIZER;
/*****************************/

ssize_t handle_with_cache(gfcontext_t *ctx, const char *path, void* arg){
	int result;
	int bytes_sent;
	shm_details *sharemem;
	// set the message queue
	mqd_t msgqueue =  mq_open(MESSAGEQUEUE,O_RDWR);
	if(msgqueue == (mqd_t)-1)
	{
		perror("mq_open");
	}
	req_struct request_data;
	
	strncpy(request_data.path,path,MAX_REQUEST_LEN);
	
	/***********************************************************/

	pthread_mutex_lock(&shm_mutex); // protecting access to the shared memory
	while(steque_isempty(&shm_queue))
	{
		pthread_cond_wait(&shm_cond,&shm_mutex);
	}
	sharemem=(shm_details*)steque_pop(&shm_queue);
	pthread_mutex_unlock(&shm_mutex);
	pthread_cond_broadcast(&shm_cond);
	
	// setting the segment id and seg size in the request
	strcpy(request_data.shm_id,sharemem->shm_id);
	request_data.segsize = sharemem->segsize;
	
	printf("Proxy worker %ld sending request %s \n",pthread_self(),path);
	// initialize semaphore
	sem_init(&sharemem->filelen_sem,1,0);		
	sem_init(&sharemem->proxysem,1,0);
	sem_init(&sharemem->cachesem,1,1);
	result = mq_send(msgqueue,(char*)&request_data,sizeof(request_data),0); // send the request to the cache
	if(result < 0)
	{
		perror("mq_send:");
	}
	
	
	
	printf("Proxy worker %ld waiting \n",pthread_self());
	bytes_sent = processResponse(sharemem,ctx);
	
	return bytes_sent;
}

// INITIALIZING THE SHARED MEMORY DESCRIPTORS AND STEQUE
void initSharedMemory(int segments,size_t segsize)
{
	
	int shmid;
	shm_details *shm;	
	steque_init(&shm_queue);
	void *ptr;
	for(int m =0 ; m<segments; m++)
	{
		char memName[10];
			snprintf(memName,10,"Shm_%d", m);
				
			shm_unlink(memName);
			shmid = shm_open(memName, O_RDWR | O_CREAT , S_IRUSR | S_IWUSR);
			
			if(shmid == -1)
			{
				perror("shm_open error:");
			}
			if(ftruncate(shmid,segsize+sizeof(shm_details)) == -1)
			{
				perror("ftruncate error:");
			}

			ptr=mmap(NULL,segsize+sizeof(shm_details),PROT_READ | PROT_WRITE,MAP_SHARED,shmid,0);
			if(ptr == MAP_FAILED)
			{
				perror("mmap error:");
			}
			shm=(shm_details *)ptr;
		 
			//printf("memname--->%s\n",memName);
			strcpy(shm->shm_id,memName);
			shm->segsize=segsize;
			shm->filesize=0;
			shm->read_len=0;
			shm->status=0;
		// adding the segment identifier to the queue
			pthread_mutex_lock(&shm_mutex);
			steque_enqueue(&shm_queue,shm);
			pthread_mutex_unlock(&shm_mutex);
	}
	printf("SHARED MEMORY INITIALIZED ..%d\n",steque_size(&shm_queue));
}

size_t processResponse(shm_details *shm,gfcontext_t *ctx)
{
	
	//printf("inside process response ->%ld\n",shm->filesize);
	sem_wait(&shm->filelen_sem);
	printf(" Proxy worker --> %ld out of wait\n",pthread_self());
	 if(shm->status == 200)
	{
		
		printf(":GF_OK\n");
		 gfs_sendheader(ctx,GF_OK,shm->filesize);	
		
		size_t bytes_transferred =0;
		size_t bytes_sent=0;
		
		while(bytes_transferred < shm->filesize)
		{
			sem_wait(&shm->proxysem);
			//printf("Read length from sharemem %ld\n",shm->read_len);
			bytes_sent = gfs_send(ctx,shm->data,shm->read_len);
			
			if(bytes_sent <=0)
			{
				perror("Error sending data to client\n");
				break;
			}
			bytes_transferred +=bytes_sent;
			//printf("file data---->%s\n",shm->data);
			sem_post(&shm->cachesem);
		}
		printf(" Proxy worker %ld finished Bytes_transferred  : %ld\n ",pthread_self(),bytes_transferred);
		queueMemory(shm,shm->shm_id);
		return bytes_transferred;
		
	}
	else if(shm->status == 404)
	{
		
		printf("GF_FILE_NOT_FOUND\n");
		queueMemory(shm,shm->shm_id);
		return gfs_sendheader(ctx,GF_FILE_NOT_FOUND,0);
	}
	return 0;
}
void queueMemory(shm_details *mem, char * memName)
{
	
	//printf("Cleaning up segment - %s\n",mem->shm_id);
	
	mem->read_len=0;
	mem->filesize=0;
	//printf("Queue size before->%d\n",steque_size(&shm_queue));
	pthread_mutex_lock(&shm_mutex);
	steque_enqueue(&shm_queue,mem);
	pthread_mutex_unlock(&shm_mutex);
	pthread_cond_broadcast(&shm_cond);
	printf("Added the segment back in steque..%d\n",steque_size(&shm_queue));
	
}