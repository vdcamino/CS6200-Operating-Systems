#include <stdlib.h>

#include "gfclient-student.h"

#define MAX_THREADS 1572
#define PATH_BUFFER_SIZE 1536

#define USAGE                                                    \
  "usage:\n"                                                     \
  "  webclient [options]\n"                                      \
  "options:\n"                                                   \
  "  -h                  Show this help message\n"               \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n"  \
  "  -p [server_port]    Server port (Default: 10880)\n"         \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n"\
  "  -t [nthreads]       Number of threads (Default 16 Max: 1572)\n"       \
  "  -r [num_requests]   Request download total (Default: 12)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"nrequests", required_argument, NULL, 'r'},
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"workload-path", required_argument, NULL, 'w'},
    {"nthreads", required_argument, NULL, 't'},
    {NULL, 0, NULL, 0}};

static void Usage() { fprintf(stderr, "%s", USAGE); }

static void localPath(char *req_path, char *local_path) {
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path) {
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while (NULL != (cur = strchr(prev + 1, '/'))) {
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)) {
      if (errno != EEXIST) {
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if (NULL == (ans = fopen(&path[0], "w"))) {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

// helper function to clear the different buffers we use
void clear_buffer(char *buffer, int bufsize){
  memset(buffer, '\0', bufsize);
}

// declare as global variables so it can be easily accessed by the different functions
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
steque_t* work_queue;
int nrequests_done = 0; 
unsigned short port = 10880;
int returncode = 0;
int nthreads = 16;
int nrequests = 12;
char *server = "localhost";

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;
  fwrite(data, 1, data_len, file);
}

// create threads
void set_pthreads(pthread_t* threads){
  int res;
  for (int i = 0; i < nthreads; i++) {
    res = pthread_create(&threads[i], NULL, thread_handle_req, NULL);
    if (res != 0)
      exit(34);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  char *workload_path = "workload.txt";
  char *req_path = NULL;
  int option_char = 0;
  
  setbuf(stdout, NULL);  // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:n:hs:t:r:w:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'r': // nrequests
        nrequests = atoi(optarg);
        break;
      case 's':  // server
        server = optarg;
        break;
     case 't':  // nthreads
        nthreads = atoi(optarg);
        break;
      default:
        Usage();
        exit(1);
      case 'p':  // port
        port = atoi(optarg);
        break;
      case 'n':
        break;
      case 'w':  // workload-path
        workload_path = optarg;
        break;

      case 'h':  // help
        Usage();
        exit(0);
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }
  if (port > 65331) {
    fprintf(stderr, "Invalid port number\n");
    exit(EXIT_FAILURE);
  }
  if (nthreads < 1 || nthreads > MAX_THREADS) {
    fprintf(stderr, "Invalid amount of threads\n");
    exit(EXIT_FAILURE);
  }
  gfc_global_init();

  // add your threadpool creation here
  // create work queue 
  work_queue = (steque_t *)malloc(sizeof(steque_t));
  steque_init(work_queue);

  // create threads vector
  pthread_t* threads = (pthread_t *)malloc(nthreads*sizeof(pthread_t));
  set_pthreads(threads);
  
  for (int i = 0; i < nrequests; i++) {
    /* Note that when you have a worker thread pool, you will need to move this
     * logic into the worker threads */
    req_path = workload_get_path();

    if (strlen(req_path) > PATH_BUFFER_SIZE) {
      fprintf(stderr, "Request path exceeded maximum of %d characters\n.", PATH_BUFFER_SIZE);
      exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&mutex);
    steque_enqueue(work_queue, req_path);
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
  }

  // boss waiting all the requests to be processed
  pthread_mutex_lock(&mutex);
  // conditional wait ends when the queue is not empty 
  while (nrequests_done != nrequests) {
    pthread_cond_wait(&cond, &mutex);
  }
  pthread_mutex_unlock(&mutex);
  pthread_cond_broadcast(&cond);

  // worker threads
  for(int j = 0; j < nthreads; j++) {
    if ((pthread_join(threads[j], NULL)) != 0)
      exit(19);
  }

  gfc_global_cleanup(); // clean global variables             
  free(threads); // free malloc pointers
  free(work_queue);
  return 0;
}

// handler
void *thread_handle_req() {
  char *req_obj;
  for (int i = 0; i < nrequests; i++){
    pthread_mutex_lock(&mutex);
    while (steque_isempty(work_queue)){
      if (nrequests_done == nrequests) {
        pthread_mutex_unlock(&mutex);
        return 0;
      }
      pthread_cond_wait(&cond, &mutex);
    }
    req_obj = steque_pop(work_queue);
    pthread_mutex_unlock(&mutex); 
    pthread_cond_signal(&cond);
    set_request_main_code(req_obj); // skeleton code for each worker thread
    pthread_mutex_lock(&mutex);
    nrequests_done += 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond);
  }
  pthread_exit(NULL);
}

// code from the main function provided as skeleton code. Used for each worker thread
void set_request_main_code(char* filepath){
  gfcrequest_t* gfr;
  char local_path[BUFSIZE];
  FILE *file = NULL;
  localPath(filepath, local_path);

  file = openFile(local_path);

  gfr = gfc_create();
  gfc_set_server(&gfr, server);
  gfc_set_path(&gfr, filepath);
  gfc_set_port(&gfr, port);
  gfc_set_writefunc(&gfr, writecb);
  gfc_set_writearg(&gfr, file);

  fprintf(stdout, "Requesting %s%s\n", server, filepath);

  if (0 > (returncode = gfc_perform(&gfr))) {
    fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
    fclose(file);
    if (0 > unlink(local_path))
      fprintf(stderr, "warning: unlink failed on %s\n", local_path);
  } else {
    fclose(file);
  }

  if (gfc_get_status(&gfr) != GF_OK) {
    if (0 > unlink(local_path))
      fprintf(stderr, "warning: unlink failed on %s\n", local_path);
  }

  fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
  fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr), gfc_get_filelen(&gfr));

  gfc_cleanup(&gfr);
}