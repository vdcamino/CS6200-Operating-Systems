
#include "gfserver-student.h"
#include <pthread.h>
#include "steque.h"
#include <unistd.h>

#define DATA_BUFFER 5000
#define USAGE                                                               \
  "usage:\n"                                                                \
  "  gfserver_main [options]\n"                                             \
  "options:\n"                                                              \
  "  -t [nthreads]       Number of threads (Default: 8)\n"                  \
  "  -p [listen_port]    Listen port (Default: 30605)\n"                    \
  "  -m [content_file]   Content file mapping keys to content files\n"      \
  "  -d [delay]          Delay in content_get, default 0, range 0-5000000 " \
  "(microseconds)\n "                                                       \
  "  -h                  Show this help message.\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"content", required_argument, NULL, 'm'},
    {"delay", required_argument, NULL, 'd'},
    {"nthreads", required_argument, NULL, 't'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

extern unsigned long int content_delay;

extern gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg);

static void _sig_handler(int signo) {
  if ((SIGINT == signo) || (SIGTERM == signo)) {
    exit(signo);
  }
}

pthread_mutex_t steque_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for Steque
pthread_cond_t cv_server = PTHREAD_COND_INITIALIZER; // Server waits on this CV
steque_t * steque_queue;

void *worker(void*args); // Worker function prototype

void enqueue_item(gfcontext_t ***ctx, const char *path) {

  pthread_mutex_lock(&steque_mutex); // LOCK

  queue_item * item = (queue_item*)malloc(sizeof(queue_item));
	item->ctx = **ctx;
	memset(item->path, 0, MAX_FILE_NAME_LEN);
	memcpy(item->path, path, strlen(path));
	steque_enqueue(steque_queue, item);

  // COND SIGNAL TO WORKER CV
  pthread_cond_signal(&cv_server);	

  pthread_mutex_unlock(&steque_mutex); // UNLOCK

}

/* Main ========================================================= */
int main(int argc, char **argv) {
  int option_char = 0;
  unsigned short port = 30605;
  char *content_map = "content.txt";
  gfserver_t *gfs = NULL;
  int nthreads = 8;

  setbuf(stdout, NULL);

  if (SIG_ERR == signal(SIGINT, _sig_handler)) {
    fprintf(stderr, "Can't catch SIGINT...exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (SIG_ERR == signal(SIGTERM, _sig_handler)) {
    fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
    exit(EXIT_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:t:rhm:d:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 't':  // nthreads
        nthreads = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 'p':  // listen-port
        port = atoi(optarg);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'm':  // file-path
        content_map = optarg;
        break;
      case 'd':  // delay
        content_delay = (unsigned long int)atoi(optarg);
        break;
    }
  }

  /* not useful, but it ensures the initial code builds without warnings */
  if (nthreads < 1) {
    nthreads = 1;
  }

  if (content_delay > 5000000) {
    fprintf(stderr, "Content delay must be less than 5000000 (microseconds)\n");
    exit(__LINE__);
  }

  content_init(content_map);

  /* Initialize thread management */
  pthread_t tid[nthreads];
  steque_queue = (steque_t *)malloc(sizeof(steque_t));
  steque_init(steque_queue);

  for(int i = 0; i < nthreads; i++) {
    if (pthread_create(&tid[i],NULL,worker,NULL) != 0) {
      perror("Error occured in creating pThreads: ");
      return -1;
    }
  }

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 21);
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, NULL);  // doesn't have to be NULL!

  /*Loops forever*/
  gfserver_serve(&gfs);

  // Cleanup
  free(gfs);
  free(tid);
  steque_destroy(steque_queue);
  content_destroy();

  // Joining the threads
  for(int i = 0; i < nthreads; i++) {
    if (pthread_join(tid[i],NULL) < 0) {
      perror("Error occured in joining pThreads: ");
    }
  }

}

// Worker thread implementation
void *worker(void*args) {
  while (1) {

    int fd;
    struct stat stats;
    ssize_t filelen;

    pthread_mutex_lock(&steque_mutex); // LOCK

    // Wait as long as there's no work (Queue is empty)
    while (steque_isempty(steque_queue))
    {
      pthread_cond_wait(&cv_server, &steque_mutex);
    }
    // POP FROM QUEUE AND THEN PROCESS IT (SEND IT TO CLIENT)
    queue_item * item = (queue_item*)steque_pop(steque_queue);

    gfcontext_t * context = item->ctx;

    pthread_mutex_unlock(&steque_mutex); // UNLOCK

    //////// DO THE ACTUAL SENDING TO CLIENT
    //// SENDING THE HEADER
    // Check if file exists for FILE_NOT_FOUND
    if ((fd = content_get(item->path)) == -1) {
      gfs_sendheader(&context, GF_FILE_NOT_FOUND, 0);
      continue;
    }
    // Checking file status using thread safe function fstat and sending error on error
    // Read that lseek is not thread safe and will return 0 on shared memory
    if (fstat(fd, &stats) == -1) {
        gfs_sendheader(&context, GF_ERROR, 0);
        perror("Fstat returned error to get the file len");
        close(fd);
        continue;
    }
    filelen = (ssize_t)stats.st_size;
    gfs_sendheader(&context, GF_OK, filelen);

    //// SENDING THE DATA
    char data[DATA_BUFFER];
    memset(data, 0, DATA_BUFFER);
    int total_data_out = 0;
    int data_buffer = 0;
    while ((data_buffer = pread(fd, data, DATA_BUFFER, total_data_out)) > 0) {

      gfs_send(&context, data, data_buffer);

      total_data_out += data_buffer;
      // Resetting the data variable buffer
      memset(data, 0, DATA_BUFFER);
    }

    // free(item->ctx);
    // item->ctx = NULL;
    free(context);
    context = NULL;
    free(item);
    item = NULL;
  }

  return 0;

}