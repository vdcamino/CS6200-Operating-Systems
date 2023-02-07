#include "gfserver-student.h"
#include <pthread.h>
#include "steque.h"
#define USAGE                                                               \
  "usage:\n"                                                                \
  "  gfserver_main [options]\n"                                             \
  "options:\n"                                                              \
  "  -t [nthreads]       Number of threads (Default: 7)\n"                  \
  "  -p [listen_port]    Listen port (Default: 20121)\n"                    \
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
//////////////////////////////////////////////////////////////
pthread_mutex_t mutex =  PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond=  PTHREAD_COND_INITIALIZER;
 steque_t* queue;
//////////////////////////////////////////////////////////////

/* Main ========================================================= */
int main(int argc, char **argv) {
  int option_char = 0;
  unsigned short port = 20121;
  char *content_map = "content.txt";
  gfserver_t *gfs=NULL;
  int nthreads = 3;

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
  queue=malloc(sizeof(steque_t));
  steque_init(queue);
  /* Initialize thread management */
 
  init_threads(nthreads);
  
  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 20);
  
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, queue);  // doesn't have to be NULL!

  /*Loops forever*/
  gfserver_serve(&gfs);
  cleanup_threads();
   
}

void init_threads(size_t nthreads)
{
  //workers= malloc(sizeof(pthread_t) * nthreads);
  pthread_t workers[nthreads];
  int i,result;
  for(i=0;i<nthreads;i++)
  {
    if((result=pthread_create(&workers[i],NULL,handleRequest,NULL))<0)
    {
      perror("Error launching threads");
      break;
    }
  }
}

void* handleRequest(void*arg)
{
    //printf("Worker %ld reporting..\n",worker);
  gfcontext_t *ctx=NULL;
  int fildes,filstats;
  int BUFSIZE=2012;
  char buffer[BUFSIZE];
  memset(&buffer,'\0',BUFSIZE);
  struct stat fils;
  requestInfo *request=NULL;
  /////////////
  while(1)
  {
            
            pthread_t t=pthread_self();
               pthread_mutex_lock(&mutex);
              while(steque_isempty(queue))
              {
                    printf("Worker %ld waiting ....\n",t);
                    pthread_cond_wait(&cond,&mutex);
                    printf("Worker %ld after condwait\n",t);
              }
              printf("Worker %ld about to pop queue\n",t);
              request =steque_pop(queue);
              pthread_mutex_unlock(&mutex);
              pthread_cond_signal(&cond);
              ctx=request->ctx;
             if((fildes =content_get(request->filepath))<0)
              {
                gfs_sendheader(&ctx,GF_FILE_NOT_FOUND,0);
                break;
              }
              if((filstats=fstat(fildes,&fils))<0)
              {
                gfs_sendheader(&ctx,GF_ERROR,0);
                close(fildes);
                break;
              }
                pthread_t worker=pthread_self();
                printf("Worker %ld Serving %s bytes of %ld\n",t,request->filepath,fils.st_size);
                gfs_sendheader(&ctx,GF_OK,fils.st_size);  
                /**********/
                size_t offset=0;       
                size_t bytes_read =0;
                size_t bytes_sent=0;
                
              //printf("Worker %ld transferring  %s\n",worker,request->filepath);
                while(offset < fils.st_size)
                {
                  bytes_read = pread(fildes,buffer,1024,offset);
                  if(bytes_read == -1){perror("Error reading file");break;}
                  if(bytes_read ==0){break;}
                  bytes_sent= gfs_send(&ctx,buffer,bytes_read);
                  offset+=bytes_sent;
                   //memset(&buffer,'\0',BUFSIZE);
                }
              printf("Worker %ld successfully completed \n",worker);
               free(ctx);
               free(request);
          }
          free(ctx);
        free(request);
        
  return 0;
}

void cleanup_threads()
{
  //free(workers);
   content_destroy();
   steque_destroy(queue);
}