#include "gfclient-student.h"
#include <pthread.h>
#include "steque.h"

#define MAX_THREADS (2105)

#define USAGE                                                   \
  "usage:\n"                                                    \
  "  webclient [options]\n"                                     \
  "options:\n"                                                  \
  "  -h                  Show this help message\n"              \
  "  -r [num_requests]   Request download total (Default: 5)\n" \
  "  -p [server_port]    Server port (Default: 30605)\n"        \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n" \
  "  -t [nthreads]       Number of threads (Default 8)\n"       \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"nthreads", required_argument, NULL, 't'},
    {"nrequests", required_argument, NULL, 'r'},
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"workload-path", required_argument, NULL, 'w'},
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

pthread_mutex_t steque_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for Steque
pthread_mutex_t jobsleft_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex lock for modifying jobsleft thread safe-ly
pthread_cond_t cv_client = PTHREAD_COND_INITIALIZER; // Server waits on this CV
pthread_cond_t cv_jobsleft = PTHREAD_COND_INITIALIZER; // Jobsleft CV to broadcast a close of treads signaling end of processing
steque_t * steque_queue; // Steque to hold outgoing requests
void *worker(void*args); // Worker function prototype
static int jobsleft;

// Argument structure to pass values to the worker function
typedef struct worker_args {
    int nrequests;
    char *server;
    int returncode;
    unsigned short port;
} worker_args;

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;

  fwrite(data, 1, data_len, file);
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  char *server = "localhost";
  unsigned short port = 30605;
  char *workload_path = "workload.txt";
  int option_char = 0;
  int nrequests = 5;
  int nthreads = 8;
  int returncode = 0;
  //gfcrequest_t *gfr = NULL;
  //FILE *file = NULL;
  char *req_path = NULL;
  //char local_path[1066];
  int i = 0;

  setbuf(stdout, NULL);  // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:n:hs:w:r:t:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'h':  // help
        Usage();
        exit(0);
        break;
      case 'r':
        nrequests = atoi(optarg);
        break;
      case 'n':  // nrequests
        break;
      case 'p':  // port
        port = atoi(optarg);
        break;
      default:
        Usage();
        exit(1);
      case 's':  // server
        server = optarg;
        break;
      case 't':  // nthreads
        nthreads = atoi(optarg);
        break;
      case 'w':  // workload-path
        workload_path = optarg;
        break;
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  if (nthreads < 1) {
    nthreads = 1;
  }
  if (nthreads > MAX_THREADS) {
    nthreads = MAX_THREADS;
  }

  gfc_global_init();

  /* add your threadpool creation here */
  pthread_t tid[nthreads];
  steque_queue = (steque_t *)malloc(sizeof(steque_t));
  steque_init(steque_queue);
  jobsleft = nrequests;

  worker_args * arguments = (worker_args *)malloc(sizeof(worker_args));

  arguments->nrequests = nrequests;
  arguments->port = port;
  arguments->returncode = returncode;
  arguments->server = server;

  for(int i = 0; i < nthreads; i++) {
    if (pthread_create(&tid[i],NULL,worker,(void *)arguments) != 0) {
      perror("Error occured in creating pThreads: ");
      return -1;
    }
  }

  /* Making the number of requests to get the path and add it to the queue
  and signal the worker threads to process it */
  for (i = 0; i < nrequests; i++) {

    // Get workload path to add to the queue and some error checking
    req_path = workload_get_path();

    if (strlen(req_path) > 1024) {
      fprintf(stderr, "Request path exceeded maximum of 1024 characters\n.");
      exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&steque_mutex); //LOCK

    // Enqueue the workload path onto the steque
    steque_enqueue(steque_queue, req_path);

    //Signal the condition variable so that the workers waiting can begin work
    pthread_cond_signal(&cv_client);

    pthread_mutex_unlock(&steque_mutex); // UNLOCK

  }

  //////// JobsLeft Mutex waiting to broadcast jobs after nrequests are processed
  pthread_mutex_lock(&jobsleft_mutex);// LOCK
  // Worker Wait until there's a new request in the queue
  while (jobsleft > 0) {
    pthread_cond_wait(&cv_jobsleft, &jobsleft_mutex);
  }
  // Broadcast active threads post-processing to wake them up and exit from their wait loop
  pthread_cond_broadcast(&cv_client);
  pthread_mutex_unlock(&jobsleft_mutex);// UNLOCK   

  // while (jobsleft > 0) {
  //   // Done processing everything
  //   if (jobsleft == 0) {
  //     // Broadcasting rest of the threads to exit if they're still active
  //     pthread_cond_broadcast(&cv_client);
  //     break;
  //   }
  //   sleep(5);
  // }

  // Joining the threads
  for(int i = 0; i < nthreads; i++) {
    if ((pthread_join(tid[i],NULL)) < 0) {
      perror("Error occured in joining pThreads: ");
    }
  }

  gfc_global_cleanup();  // use for any global cleanup for AFTER your thread
                         // pool has terminated.
  free(arguments);
  arguments = NULL;                         
  steque_destroy(steque_queue);
  free(steque_queue);
  return 0;
}

// Worker function definition
void *worker(void*args) {

  // Initializing variables for the client worker
  int nrequests = ((struct worker_args *)args)->nrequests;
  unsigned short port = ((struct worker_args *)args)->port;
  int returncode = ((struct worker_args *)args)->returncode;
  char * server = ((struct worker_args *)args)->server;
  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;  
  char local_path[1066];
  char *req_path;


  for (int i = 0; i < nrequests; i++) {

    pthread_mutex_lock(&steque_mutex);// LOCK

    // Worker Wait until there's a new request in the queue
    while (steque_isempty(steque_queue)) {

      // Early exit logic if all the jobs have been processed from the queue
      if (jobsleft <= 0) {
        pthread_mutex_unlock(&steque_mutex); // UNLOCK EARLY, No jobs
        gfc_cleanup(&gfr);
        //pthread_exit(NULL);
        return 0;
      }

      pthread_cond_wait(&cv_client, &steque_mutex);
    }

    req_path = steque_pop(steque_queue);

    pthread_mutex_unlock(&steque_mutex);// UNLOCK    

    /* CODE FROM INT MAIN */
    localPath(req_path, local_path);

    file = openFile(local_path);

    gfr = gfc_create();
    gfc_set_server(&gfr, server);
    gfc_set_path(&gfr, req_path);
    gfc_set_port(&gfr, port);
    gfc_set_writefunc(&gfr, writecb);
    gfc_set_writearg(&gfr, file);

    fprintf(stdout, "Requesting %s%s\n", server, req_path);

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
    fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
            gfc_get_filelen(&gfr));

    gfc_cleanup(&gfr);
    /* END OF CANNED CODE FROM INT MAIN*/

    // Decrementing jobsleft
    pthread_mutex_lock(&jobsleft_mutex);
    jobsleft--;
    pthread_cond_signal(&cv_jobsleft);
    pthread_mutex_unlock(&jobsleft_mutex);

    //pthread_exit(NULL);
  }

  return 0;
}