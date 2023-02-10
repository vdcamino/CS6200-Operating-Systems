
#include <stdlib.h>

#include "gfserver-student.h"

#define BUFSIZE 512
#define USAGE                                                               \
  "usage:\n"                                                                \
  "  gfserver_main [options]\n"                                             \
  "options:\n"                                                              \
  "  -h                  Show this help message.\n"                         \
  "  -t [nthreads]       Number of threads (Default: 20)\n"                 \
  "  -m [content_file]   Content file mapping keys to content files (Default: content.txt\n"      \
  "  -p [listen_port]    Listen port (Default: 10880)\n"                    \
  "  -d [delay]          Delay in content_get, default 0, range 0-5000000 " \
  "(microseconds)\n "

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"delay", required_argument, NULL, 'd'},
    {"nthreads", required_argument, NULL, 't'},
    {"content", required_argument, NULL, 'm'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

extern unsigned long int content_delay;

extern gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg);

static void _sig_handler(int signo) {
  if ((SIGINT == signo) || (SIGTERM == signo)) {
    exit(signo);
  }
}

// helper function to clear the different buffers we use
void clear_buffer(char *buffer, int bufsize){
  memset(buffer, '\0', bufsize);
}

// initialize global variables
pthread_mutex_t mutex =  PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond =  PTHREAD_COND_INITIALIZER;
steque_t* work_queue;

/* Main ========================================================= */
int main(int argc, char **argv) {
  gfserver_t *gfs = NULL;
  int nthreads = 20;
  int option_char = 0;
  unsigned short port = 10880;
  char *content_map = "content.txt";

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
  while ((option_char = getopt_long(argc, argv, "p:d:rhm:t:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'h':  /* help */
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 'p':  /* listen-port */
        port = atoi(optarg);
        break;
      case 't':  /* nthreads */
        nthreads = atoi(optarg);
        break;
      case 'm':  /* file-path */
        content_map = optarg;
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'd':  /* delay */
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

  // Initialize threads and work queue 
  work_queue = (steque_t*)malloc(sizeof(*work_queue));
  steque_init(work_queue);
  set_pthreads(nthreads);
  
  // create server
  gfs = gfserver_create();
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 20);
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, work_queue);
  gfserver_serve(&gfs);
}


void set_pthreads(size_t nthreads){
  pthread_t threads[nthreads];
  int res = pthread_mutex_init(&mutex, NULL);
  if (res != 0)
    exit(92);
  for (int i = 0; i < nthreads; i++) {
    res = pthread_create(&threads[i], NULL, thread_handle_req, NULL);
    if (res != 0)
      exit(34);
  }
}

void *thread_handle_req(void *arg) {
  int fd, fstats;
  char buf[BUFSIZE];
  struct stat finfo;
  steque_request *req;
  size_t total_bts_sent, bts_sent, bts_read, file_len;

  clear_buffer(buf, BUFSIZE);

  for(;;){
    // mutex lock
    if (pthread_mutex_lock(&mutex) != 0)
      exit(12);

    // do nothing while queue is empty
    while (steque_isempty(work_queue))
      pthread_cond_wait(&cond, &mutex);

    // get a request from the queue 
    req = steque_pop(work_queue);

    // mutex unlock
    if (pthread_mutex_unlock(&mutex))
      exit(29);

    // signal to other threads
    pthread_cond_signal(&cond);

    // send file 
    // start by getting the filepath 
    fd = content_get(req->filepath);
    // check if you have found the file 
    if (fd == -1){
      gfs_sendheader(&req->context, GF_FILE_NOT_FOUND, 0);
      break;
    } 
    // file was found, now get stats
    fstats = fstat(fd, &finfo);
    // check if error when getting stats
    if (fstats == -1) {
      gfs_sendheader(&req->context, GF_ERROR, 0);
      close(fd);
      break;
    }

    // everything ok, send header
    gfs_sendheader(&req->context, GF_OK, finfo.st_size);
    
    // send everything 
    total_bts_sent = 0;
    bts_sent = 0;
    bts_read = 0;
    file_len = finfo.st_size;
    do {
      clear_buffer(buf, BUFSIZE);
      bts_read = pread(fd, buf, BUFSIZE, total_bts_sent);
      if(!(bts_read > 0))
        break;
      bts_sent = gfs_send(&req->context, buf, bts_read);
      total_bts_sent += bts_sent;
    } while(file_len > total_bts_sent);

    // free resources
    free(req);
  }
  free(req);
  return 0;
}