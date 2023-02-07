#include "gfclient-student.h"
#include "steque.h"
#include <pthread.h>
#define MAX_THREADS (2012)

#define USAGE                                                   \
  "usage:\n"                                                    \
  "  webclient [options]\n"                                     \
  "options:\n"                                                  \
  "  -h                  Show this help message\n"              \
  "  -r [num_requests]   Request download total (Default: 2)\n" \
  "  -p [server_port]    Server port (Default: 20121)\n"        \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n" \
  "  -t [nthreads]       Number of threads (Default 2)\n"       \
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

  // Make the directory if it isn't there 
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

//Callbacks ========================================================= 
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;

  fwrite(data, 1, data_len, file);
}
/****************************************************************/
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond= PTHREAD_COND_INITIALIZER;

steque_t* requestQueue;
char *server = "localhost";
  unsigned short port = 20121;
   int nrequests = 1;
   int nthreads = 1;
   int requests_served;
   int requests_enqueued;
/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  
  char *workload_path = "workload.txt";
  int option_char = 0;
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
   requestQueue =malloc(sizeof(steque_t));
   steque_init(requestQueue);
  // CREATING THE THREAD WORKER POOL
  pthread_t *workers =malloc(sizeof(pthread_t) *nthreads);
  int i,result;
  for(i=0;i<nthreads;i++)
  {
      result=pthread_create(&workers[i],NULL,processRequest,&workers[i]);
      if(result !=0){perror("Error launching threads\n");break;}
    
  }
  
  // BOSS ENQUEING THE REQUESTS
  char *req_path = NULL;
  for (i = 0; i < nrequests; i++) {
    
  
    req_path = workload_get_path();

    if (strlen(req_path) > 1024) {
      fprintf(stderr, "Request path exceeded maximum of 1024 characters\n.");
      exit(EXIT_FAILURE);
    }
     pthread_mutex_lock(&mutex);
    printf("Boss Enqueuing a request path %s\n",req_path);
    steque_enqueue(requestQueue,req_path);
    requests_enqueued+=1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);

 }
      //BOSS WAITING UNTIL ALL THE REQUESTS ARE ENQUEUED AND PROCESSED
      pthread_mutex_lock(&mutex);
       while(requests_served < nrequests && requests_served < nrequests)
        {printf("Boss waiting to complete %d more requests..\n",requests_served);
          pthread_cond_wait(&cond,&mutex);
        }
      pthread_mutex_unlock(&mutex);
     pthread_cond_broadcast(&cond);
    // WORKER THREADS JOINING THE BOSS ONCE WORK IS COMPLETED
  for(int z=0;z<nthreads;z++)
  {
      int ptr;
      
       pthread_join(workers[z],(void**)&ptr);
       printf("Worker %ld joining the boss---->\n",workers[z]);
  }
    free(workers);
  gfc_global_cleanup();  // use for any global cleanup for AFTER your thread
                         // pool has terminated.

  return 0;
}

// DE-QUEUEING THE REQUEST FROM THE STEQUE
void* processRequest(void *arg)
{
  
  while(1)
  {
        long threadId=*(long*)arg;
        printf("Thread %ld reporting....\n",threadId);  
        char *req_path = NULL;
              pthread_mutex_lock(&mutex);
              
                while(steque_isempty(requestQueue))
                {
                  if(requests_enqueued == nrequests && requests_served == nrequests)
                  {
                    printf("Breaking the loop\n");
                    pthread_mutex_unlock(&mutex);
                    return 0;
                  }
                  printf("Thread %ld waiting for request..\n",threadId);
                  pthread_cond_wait(&cond,&mutex);
                }
            
            req_path =steque_pop(requestQueue);
            pthread_mutex_unlock(&mutex);
            pthread_cond_signal(&cond);
            sendRequest(req_path);
          
  }
  
     return 0;
}

void sendRequest(char *path)
{

  FILE *file =NULL;
  char local_path[2000];
  gfcrequest_t *gfr=NULL;
  pthread_t worker=pthread_self();
  localPath(path, local_path);
  int returncode;
    file = openFile(local_path);

    gfr = gfc_create();
    gfc_set_server(&gfr, server);
    gfc_set_path(&gfr, path);
    gfc_set_port(&gfr, port);
    gfc_set_writefunc(&gfr, writecb);
    gfc_set_writearg(&gfr, file);

    fprintf(stdout, "Worker %ld Requesting %s%s\n", worker,server, path);

    if (0 > (returncode = gfc_perform(&gfr))) {
      fprintf(stdout, "gfc_perform returned error %d\n", returncode);
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

    //fprintf(stdout, "Worker %ld Status: %s\n", worker,gfc_strstatus(gfc_get_status(&gfr)));
    fprintf(stdout, "Worker %ld Received %zu of %zu bytes\n",worker,gfc_get_bytesreceived(&gfr),
            gfc_get_filelen(&gfr));

    
    pthread_mutex_lock(&mutex); 
      requests_served+=1;
       pthread_mutex_unlock(&mutex);
       pthread_cond_broadcast(&cond);
    gfc_cleanup(&gfr);

  }

