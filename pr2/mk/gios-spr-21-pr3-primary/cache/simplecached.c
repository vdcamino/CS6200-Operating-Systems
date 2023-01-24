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
#include <sys/mman.h>

#include "gfserver.h"
#include "cache-student.h"
#include "shm_channel.h"
#include "simplecache.h"

#if !defined(CACHE_FAILURE)
#define CACHE_FAILURE (-1)
#endif // CACHE_FAILURE

#define MAX_CACHE_REQUEST_LEN 5041

pthread_mutex_t steque_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for Steque
pthread_cond_t cv_server = PTHREAD_COND_INITIALIZER;	  // Server waits on this CV
steque_t *steque_queue;

static void _sig_handler(int signo)
{
	if (signo == SIGTERM || signo == SIGINT)
	{
		// you should do IPC cleanup here
		// Deleting Mutex and CV resources
		pthread_cond_destroy(&cv_server);
		pthread_mutex_destroy(&steque_mutex);
		// Deleting Shared Memory Message queue resources
		key_t mqkey;
		int msqid;
		mqkey = ftok("/etc", 'Q');
		if ((msqid = msgget(mqkey, IPC_CREAT | 0660)) != -1)
		{
			msgctl(msqid, IPC_RMID, 0);
		}
		// Deleting CTX Request Message queue resources
		mqkey = ftok("/etc", 'C');
		if ((msqid = msgget(mqkey, IPC_CREAT | 0660)) != -1)
		{
			msgctl(msqid, IPC_RMID, 0);
		}
		exit(signo);
	}
}

unsigned long int cache_delay;

#define USAGE                                                                                            \
	"usage:\n"                                                                                           \
	"  simplecached [options]\n"                                                                         \
	"options:\n"                                                                                         \
	"  -c [cachedir]       Path to static files (Default: ./)\n"                                         \
	"  -t [thread_count]   Thread count for work queue (Default is 7, Range is 1-31415)\n"               \
	"  -d [delay]          Delay in simplecache_get (Default is 0, Range is 0-5000000 (microseconds)\n " \
	"  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
	{"cachedir", required_argument, NULL, 'c'},
	{"nthreads", required_argument, NULL, 't'},
	{"help", no_argument, NULL, 'h'},
	{"hidden", no_argument, NULL, 'i'},		 /* server side */
	{"delay", required_argument, NULL, 'd'}, // delay.
	{NULL, 0, NULL, 0}};

void Usage()
{
	fprintf(stdout, "%s", USAGE);
}

void *worker(void *args);

int main(int argc, char **argv)
{
	int nthreads = 7;
	char *cachedir = "locals.txt";
	char option_char;

	/* disable buffering to stdout */
	setbuf(stdout, NULL);

	while ((option_char = getopt_long(argc, argv, "id:c:hlxt:", gLongOptions, NULL)) != -1)
	{
		switch (option_char)
		{
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
			cache_delay = (unsigned long int)atoi(optarg);
			break;
		case 'i': // server side usage
		case 'l': // experimental
		case 'x': // experimental
			break;
		}
	}

	if (cache_delay > 5000000)
	{
		fprintf(stderr, "Cache delay must be less than 5000000 (us)\n");
		exit(__LINE__);
	}

	if ((nthreads > 31415) || (nthreads < 1))
	{
		fprintf(stderr, "Invalid number of threads\n");
		exit(__LINE__);
	}

	if (SIG_ERR == signal(SIGINT, _sig_handler))
	{
		fprintf(stderr, "Unable to catch SIGINT...exiting.\n");
		exit(CACHE_FAILURE);
	}

	if (SIG_ERR == signal(SIGTERM, _sig_handler))
	{
		fprintf(stderr, "Unable to catch SIGTERM...exiting.\n");
		exit(CACHE_FAILURE);
	}

	// Initialize cache
	simplecache_init(cachedir);

	// cache code gos here
	/* Initialize thread management */
	pthread_t tid[nthreads];
	steque_queue = (steque_t *)malloc(sizeof(steque_t));
	steque_init(steque_queue);

	for (int i = 0; i < nthreads; i++)
	{
		if (pthread_create(&tid[i], NULL, worker, NULL) != 0)
		{
			perror("Error occured in creating pThreads: ");
			return -1;
		}
	}

	while (1)
	{
		///////////////// Receive CTX message from the CTX request queue
		int msqid_ctx;
		struct msgbuf_ctx *mybuf_ctx = malloc(sizeof(struct msgbuf_ctx));
		key_t mqkey_ctx = ftok("/etc", 'C');
		while ((msqid_ctx = msgget(mqkey_ctx, 0660)) == -1)
		{
			// failed to create message queue
			sleep(1);
		};
		while ((msgrcv(msqid_ctx, mybuf_ctx, sizeof(struct msgbuf_ctx), CTX_MESSAGE_TYPE, 0)) == -1)
		{
			// failed to receive message from MQ
			sleep(1);
		};

		///////////////// Queue the CTX message and notify the worker threads
		pthread_mutex_lock(&steque_mutex); // LOCK

		steque_enqueue(steque_queue, mybuf_ctx);

		// COND SIGNAL TO WORKER CV
		pthread_cond_signal(&cv_server);

		pthread_mutex_unlock(&steque_mutex); // UNLOCK
	}

	// Joining the threads
	for (int i = 0; i < nthreads; i++)
	{
		if (pthread_join(tid[i], NULL) < 0)
		{
			perror("Error occured in joining pThreads: ");
		}
	}

	// Won't execute
	return 0;
}

void *worker(void *args)
{
	while (1)
	{
		///////////////// Steque to process incoming requests from handle with cache
		pthread_mutex_lock(&steque_mutex); // LOCK

		// Wait as long as there's no work (Queue is empty)
		while (steque_isempty(steque_queue))
		{
			pthread_cond_wait(&cv_server, &steque_mutex);
		}
		// POP FROM QUEUE AND THEN PROCESS IT
		struct msgbuf_ctx *mybuf_ctx;
		mybuf_ctx = steque_pop(steque_queue);

		pthread_mutex_unlock(&steque_mutex); // UNLOCK

		///////////////// Attach process to the shared memory update semaphore
		struct shm_struct *shm_struct;
		shm_struct = (struct shm_struct *)shmat(mybuf_ctx->message_text_ctx.shmid, (void *)0, 0);

		///////////////// Get Filesize of the cached file if it exists and handle missing file scenario
		int fd;
		if ((fd = simplecache_get(mybuf_ctx->message_text_ctx.path)) == -1)
		{
			// Handle case of file not in cache and return early
			shm_struct->file_size = -1;
			sem_post(&shm_struct->header_semaphore);
			free(mybuf_ctx);
			shmdt((void *)shm_struct);
			continue;
		}
		struct stat buf;
		fstat(fd, &buf);
		int size = buf.st_size;
		shm_struct->file_size = size;
		sem_post(&shm_struct->header_semaphore);

		///////////////// Sending the data in chunks over SHM with semaphore sync
		// WAIT FOR PROXY TO READ BUFFER, INITIALIZE WITH 1 TO START WRITING
		int bytes_sent = 0;
		int last_block_size;
		int num_pages;
		int block_size;
		int page_size;
		int file_size;
		int i = 1;

		file_size = shm_struct->file_size;
		page_size = mybuf_ctx->message_text_ctx.segsize - sizeof(struct shm_struct);
		last_block_size = shm_struct->file_size % page_size;
		num_pages = shm_struct->file_size / page_size + (last_block_size ? 1 : 0);

		while (bytes_sent < file_size)
		{
			// Wait on write semaphore until proxy finishes reading
			sem_wait(&shm_struct->writer_semaphore);

			// Reading data in chunks from fd into shared memory
			shm_struct->data_chunk = (void *)shm_struct + sizeof(struct shm_struct);
			memset(shm_struct->data_chunk, 0, page_size);
			block_size = pread(fd, shm_struct->data_chunk, (i == num_pages ? last_block_size : page_size), bytes_sent);

			// Setting SHM Data values for proxy to use
			bytes_sent += block_size;
			shm_struct->chunk_size = block_size;
			shm_struct->total_bytes_sent = bytes_sent;

			// Post on read semaphore for proxy to read
			sem_post(&shm_struct->reader_semaphore);
			i++;
		}

		// Freeing up resources
		free(mybuf_ctx);
		shmdt((void *)shm_struct);
	}
}

/*
///////////////// SHARED MEMORY SEMAPHORE SYNCHRO LOGIC
Initialize:
	write_semaphore = 1
	read_semaphore = 0
CACHE:
	loop:
		sem_wait(write_semaphore)
		WRITE TO SHARED MEMORY
		sem_post(read_semaphore)

PROXY:
	loop:
		sem_wait(read_semaphore)
		READ SHARED MEMORY AND GFS_SEND
		sem_post(write_semaphore)
*/

// handle_with_cache.c
// shm_channel.c
// shm_channel.h
// simplecached.c
// webproxy.c
// cache-student.h