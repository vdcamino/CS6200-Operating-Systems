#include <stdio.h>
#include <sys/signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "workload.h"
#include "steque.h"
#include "gfclient-student.h"


typedef struct steque_node{
  char *server;
  unsigned short port;
	char *req_path;
  // int nrequests;
}steque_node;

pthread_cond_t mutex_cv=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex;
steque_t *req_queue;
int total_requests;
void *threadFunc(void *threadid);

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  webclient [options]\n"                                                     \
"options:\n"                                                                  \
"  -h                  Show this help message\n"                              \
"  -n [num_requests]   Requests download per thread (Default: 6)\n"           \
"  -p [server_port]    Server port (Default: 50419)\n"                         \
"  -s [server_addr]    Server address (Default: 127.0.0.1)\n"                   \
"  -t [nthreads]       Number of threads (Default 9)\n"                       \
"  -w [workload_path]  Path to workload file (Default: workload.txt)\n"       \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"help",          no_argument,            NULL,           'h'},
  {"nthreads",      required_argument,      NULL,           't'},
  {"nrequests",     required_argument,      NULL,           'n'},
  {"server",        required_argument,      NULL,           's'},
  {"port",          required_argument,      NULL,           'p'},
  {"workload-path", required_argument,      NULL,           'w'},
  {NULL,            0,                      NULL,             0}
};




static void Usage() {
	fprintf(stderr, "%s", USAGE);
}

static void localPath(char *req_path, char *local_path){
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE* openFile(char *path){
  char *cur, *prev;
  FILE *ans;

  /* Make the directory if it isn't there */
  prev = path;
  while(NULL != (cur = strchr(prev+1, '/'))){
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)){
      if (errno != EEXIST){
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if( NULL == (ans = fopen(&path[0], "w"))){
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}

/* Callbacks ========================================================= */
static void writecb(void* data, size_t data_len, void *arg){
  FILE *file = (FILE*) arg;

  fwrite(data, 1, data_len, file);
}




/* Main ========================================================= */
int main(int argc, char **argv) {
/* COMMAND LINE OPTIONS ============================================= */
  char *server = "localhost";
  unsigned short port = 50419;
  char *workload_path = "workload.txt";

  int i = 0;
  int option_char = 0;
  int nrequests = 6;
  int nthreads = 9;
  // int returncode = 0;
  // gfcrequest_t *gfr = NULL;
  // FILE *file = NULL;
  char *req_path = NULL;
  // char local_path[1066];
  total_requests = nrequests*nthreads;
  setbuf(stdout, NULL); // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "t:hrn:p:s:w:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'h': // help
        Usage();
        exit(0);
        break;
      case 'n': // nrequests
        nrequests = atoi(optarg);
        break;
      case 'p': // port
        port = atoi(optarg);
        break;
      default:
        Usage();
        exit(1);
      case 's': // server
        server = optarg;
        break;
      case 't': // nthreads
        nthreads = atoi(optarg);
        break;
      case 'w': // workload-path
        workload_path = optarg;
        break;
    }
  }

  if( EXIT_SUCCESS != workload_init(workload_path)){
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  gfc_global_init();

  req_queue = (steque_t *)malloc(sizeof(steque_t));
	steque_init(req_queue);
  pthread_t tid[nthreads];
  int t, tNum[nthreads];
  // printf("========The client starts to set pthreads ========\n");
	if (pthread_mutex_init(&mutex, NULL) != 0){
			fprintf(stderr, "\n mutex init has failed\n");
			exit(1);
	}
	for(t=0; t<nthreads; t++){
		/* create the threads; may be any number, in general */
		tNum[t] = t;
		// printf("======== The thread number is %d ========\n", t);

		if(pthread_create(&tid[t], NULL, threadFunc, &tNum[t]) != 0) {
			fprintf(stderr, "Unable to create producer thread\n");
			exit(1);
		}
    printf("------Thread %d is created!----\n",t );
	}

  // printf("------Pop out!----\n");

  /*Making the requests...*/
  for(i = 0; i < nrequests * nthreads; i++){
    req_path = workload_get_path();

    if(strlen(req_path) > 500){
      fprintf(stderr, "Request path exceeded maximum of 500 characters\n.");
      exit(EXIT_FAILURE);
    }

    steque_node* node;
    node = (steque_node*)malloc(sizeof(steque_node));
    node->server = server;
    node->port = port;
    node->req_path = req_path;
    // node->nrequests = nrequests;

    if(pthread_mutex_lock(&mutex)!=0){
      printf("\n mutex lock has failed\n");
      exit(1);
    }
    steque_enqueue(req_queue,node);
    // free(node);

    if(pthread_mutex_unlock(&mutex)!=0){
      printf("\n mutex unlock has failed\n");
      exit(1);
    }
    printf("======== Put No. %d  Path into the Queue ========\n", i);
    // pthread_cond_signal(&mutex_cv);
  }


  for(t=0; t<nthreads; t++){
    pthread_join(tid[t], NULL);
    /* create the threads; may be any number, in general */
    printf("======== Join the thread number is %d ========\n", t);
  }
  // printf("======== Total Requests is %d ========\n", total_requests);

  steque_destroy(req_queue);
  gfc_global_cleanup();

  return 0;
}


void *threadFunc(void *pArg){
  // int threadnum = *((int*)pArg);
  steque_node* req_obj;
  // char *server;
  // unsigned short port;
  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;
  // char *req_path = NULL;
  char local_path[1066];
  // int called_req = 0;
  int returncode = 0;
  // int flag = 1;
  // printf("\n ============= Step in the thread function with %d requests=========\n", nrequests);
  while(total_requests>0){
    /* If queue is empty, do not do mutex lock/unlock, just wait...*/
    if(!steque_isempty(req_queue)){
    // printf("======== Set mutex for thread ========\n");
    if(pthread_mutex_lock(&mutex)!=0){
      printf("\n mutex lock has failed\n");
      exit(1);
    }
    // printf("======== Lock mutex for thread ========\n");

        while(steque_isempty(req_queue)){
          pthread_cond_wait(&mutex_cv, &mutex);
        }
    req_obj = steque_pop(req_queue);
    total_requests = total_requests-1;
    // printf("======== Total Requests is %d ========\n", total_requests);

    if(pthread_mutex_unlock(&mutex)!=0){
      printf("\n mutex unlock has failed\n");
      exit(1);
    }

    // pthread_cond_broadcast(&mutex_cv);

    if(req_obj!=NULL){
      localPath(req_obj->req_path, local_path);

      file = openFile(local_path);

      gfr = gfc_create();
      gfc_set_server(gfr, req_obj->server);
      gfc_set_path(gfr, req_obj->req_path);
      gfc_set_port(gfr, req_obj->port);
      gfc_set_writefunc(gfr, writecb);
      gfc_set_writearg(gfr, file);

      fprintf(stdout, "Requesting %s%s\n", req_obj->server, req_obj->req_path);

      if ( 0 > (returncode = gfc_perform(gfr))){
        fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
        fclose(file);
        if ( 0 > unlink(local_path))
          fprintf(stderr, "warning: unlink failed on %s\n", local_path);
      }
      else {
          fclose(file);
      }

      if ( gfc_get_status(gfr) != GF_OK){
        if ( 0 > unlink(local_path))
          fprintf(stderr, "warning: unlink failed on %s\n", local_path);
      }


      fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(gfr)));
      fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(gfr), gfc_get_filelen(gfr));
      // called_req += 1;
      // if(called_req == req_obj->nrequests){
      //   flag = 0;
      //   // break;
      // }
      gfc_cleanup(gfr);
      // }
      free(req_obj);
      req_obj = NULL;
    }
    }
  }
  // printf("------Thread %d is quitting!----\n",threadnum );
  pthread_exit(NULL);
}
