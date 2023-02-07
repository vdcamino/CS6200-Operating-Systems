#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/signal.h>
#include <printf.h>
#include <curl/curl.h>

#include "gfserver.h"
#include "cache-student.h"
#include "shm_channel.h"
#include "simplecache.h"

#if !defined(CACHE_FAILURE)
#define CACHE_FAILURE (-1)
#endif // CACHE_FAILURE

#define MAX_CACHE_REQUEST_LEN 5041

/**************************/
mqd_t mqd = -1;
struct mq_attr attr;

/**************************/

static void _sig_handler(int signo){
	if (signo == SIGTERM || signo == SIGINT){
		mq_unlink(MESSAGEQUEUE);		
		simplecache_destroy();
		exit(signo);
	}
}

unsigned long int cache_delay;

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  simplecached [options]\n"                                                  \
"options:\n"                                                                  \
"  -c [cachedir]       Path to static files (Default: ./)\n"                  \
"  -t [thread_count]   Thread count for work queue (Default is 7, Range is 1-31415)\n"      \
"  -d [delay]          Delay in simplecache_get (Default is 0, Range is 0-5000000 (microseconds)\n "	\
"  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"cachedir",           required_argument,      NULL,           'c'},
  {"nthreads",           required_argument,      NULL,           't'},
  {"help",               no_argument,            NULL,           'h'},
  {"hidden",			 no_argument,			 NULL,			 'i'}, /* server side */
  {"delay", 			 required_argument,		 NULL, 			 'd'}, // delay.
  {NULL,                 0,                      NULL,             0}
};

void Usage() {
  fprintf(stdout, "%s", USAGE);
}


int main(int argc, char **argv) {
	int nthreads = 7;
	char *cachedir = "locals.txt";
	char option_char;

	/* disable buffering to stdout */
	setbuf(stdout, NULL);

	while ((option_char = getopt_long(argc, argv, "id:c:hlxt:", gLongOptions, NULL)) != -1) {
		switch (option_char) {
			default:
				Usage();
				exit(1);
			case 'c': //cache directory
				cachedir = optarg;
				break;
			case 'h': // help
				Usage();
				exit(0);
				break;    
			case 't': // thread-count
				nthreads = atoi(optarg);
				break;
			case 'd':
				cache_delay = (unsigned long int) atoi(optarg);
				break;
			case 'i': // server side usage
			case 'l': // experimental
			case 'x': // experimental
				break;
		}
	}

	if (cache_delay > 5000000) {
		fprintf(stderr, "Cache delay must be less than 5000000 (us)\n");
		exit(__LINE__);
	}

	if ((nthreads>31415) || (nthreads < 1)) {
		fprintf(stderr, "Invalid number of threads\n");
		exit(__LINE__);
	}

	if (SIG_ERR == signal(SIGINT, _sig_handler)){
		fprintf(stderr,"Unable to catch SIGINT...exiting.\n");
		exit(CACHE_FAILURE);
	}

	if (SIG_ERR == signal(SIGTERM, _sig_handler)){
		fprintf(stderr,"Unable to catch SIGTERM...exiting.\n");
		exit(CACHE_FAILURE);
	}

	// Initialize cache
	simplecache_init(cachedir);
	
	// create message queue / command channel
	int res= mq_unlink(MESSAGEQUEUE);
	if(res == 0)
	{
		printf("Unlinked the message queue\n");
	}
	attr.mq_curmsgs=0;
	attr.mq_flags=0;
	attr.mq_maxmsg =10;
	attr.mq_msgsize=1024;

	mqd =  mq_open(MESSAGEQUEUE,O_CREAT|O_EXCL,0666,&attr);
	if(mqd == (mqd_t)-1)
	{
		perror("mq_open:");
	}
	
	// creating the worker threads

	pthread_t workers[nthreads];
	int result;
	for(int t=0;t<nthreads;t++)
	{
		result = pthread_create(&workers[t],NULL,workerRoutine,&workers[t]);
		if(result <0)
		{
			perror("pthread_create");
			break;
		}
	}
	
	// Won't execute
	for(int z=0;z<nthreads;z++)
 	 {
    		 int ptr;
      
     		  pthread_join(workers[z],(void**)&ptr);
       		printf("Worker %ld joining the boss---->\n",workers[z]);
  	}
	return 0;
}

void * workerRoutine(void *arg)
{
			int fd;
			int numReceived=0;
			req_struct mq_msg;
	while(1)
	{
		

		numReceived = mq_receive(mqd,(char*)&mq_msg,1024,NULL);
		//printf("Number received====>%ld\n",numReceived);
		if(numReceived > 0)
		{
		
			printf("Request %s queued in %s\n",mq_msg.path,mq_msg.shm_id);
			printf("Cache worker %ld serving - %s in segment - %s\n",pthread_self(),mq_msg.path,mq_msg.shm_id);
			fd = simplecache_get(mq_msg.path);
		// cache to proxy response 
			sendResponse(fd,mq_msg.shm_id,mq_msg.segsize);

		}
	
	}
	return 0;
}

void sendResponse(int fildes, char * seg_id, size_t segsize)
{
	
	//OPENING THE SHARED MEMORY
		int shmfd;
		shmfd = shm_open(seg_id, O_RDWR,0);
		
		
		if(shmfd < 0)
		{
			perror("shm_open");
		}
		if(ftruncate(shmfd,(sizeof(shm_details) + sizeof(char) * segsize)) == -1)
			{
				perror("ftruncate error:");
			}
		shm_details *sharemem = mmap(NULL,(sizeof(shm_details) + sizeof(char) * segsize),PROT_READ | PROT_WRITE ,MAP_SHARED,shmfd,0);
		if(sharemem == MAP_FAILED)
		{
			perror("mmap:fail");
		}
		
		if(fildes < 0)
		{
			sharemem->filesize= -1;
			sharemem->status =404;
			printf("GF_FILE_NOT_FOUND\n");
			
			sem_post(&sharemem->filelen_sem);
			//munmap(sharemem,segsize);
		}
		else
		{
			size_t filsize = lseek(fildes,0,SEEK_END);
			printf("Worker %ld serving in segment %s\n",pthread_self(),sharemem->shm_id);
			sharemem->filesize = filsize;
			sharemem->status=200;
			sem_post(&sharemem->filelen_sem);
			
			printf("Cache preparing to send file...\n");
			char buffer[segsize];
			memset(&buffer,'\0',segsize);
			size_t bytes_read =0;
			size_t nbytes =0;
			size_t offset =0;
			
				
				while(bytes_read < filsize)
				{
					
					sem_wait(&sharemem->cachesem);
					//printf("Putting proxy to wait\n");
					nbytes = pread(fildes,&buffer,segsize,offset);
					if(nbytes <=0)
					{
						perror("Error reading bytes....\n");
						break;
					}
					memcpy(&sharemem->read_len,&nbytes,sizeof(nbytes));
					memcpy(&sharemem->data,&buffer,nbytes);
					bytes_read +=nbytes;
					offset +=nbytes;
					//printf(" Worker %ld copied %ld of bytes into segment - %s\n",pthread_self(),bytes_read,sharemem->shm_id);									
					sem_post(&sharemem->proxysem);
					
				}
				printf(" Worker %ld finished %ld of file bytes \n",pthread_self(),bytes_read);
				//munmap(sharemem,sharemem->segsize);
	
			
		}
	
}