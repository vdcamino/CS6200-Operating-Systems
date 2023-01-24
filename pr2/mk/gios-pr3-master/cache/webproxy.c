#include "gfserver.h"
#include "cache-student.h"


#define USAGE                                                                         \
"usage:\n"                                                                            \
"  webproxy [options]\n"                                                              \
"options:\n"                                                                          \
"  -n [segment_count]  Number of segments to use (Default is 5)\n"                    \
"  -p [listen_port]    Listen port (Default is 50419)\n"                              \
"  -t [thread_count]   Num worker threads (Default is 7, Range is 1-512)\n"           \
"  -s [server]         The server to connect to (Default: Udacity S3 Data)\n"         \
"  -z [segment_size]   The segment size (in bytes, Default: 5041).\n"                 \
"  -h                  Show this help message\n"


/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"thread-count",  required_argument,      NULL,           't'},
  {"segment-count", required_argument,      NULL,           'n'},
  {"segment-size",  required_argument,      NULL,           'z'},         
  {"server",        required_argument,      NULL,           's'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,            0}
};

extern ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg);

static gfserver_t gfs;

static void _sig_handler(int signo){
  if (signo == SIGTERM || signo == SIGINT){
    gfserver_stop(&gfs);
    exit(signo);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  int i;
  int option_char = 0;
  unsigned short port = 50419;
  unsigned short nworkerthreads = 7;
  unsigned int nsegments = 5;
  size_t segsize = 5014;
  const char *server = "s3.amazonaws.com/content.udacity-data.com";

  /* disable buffering on stdout so it prints immediately */
  setbuf(stdout, NULL);

  if (signal(SIGINT, _sig_handler) == SIG_ERR) {
    fprintf(stderr,"Can't catch SIGINT...exiting.\n");
    exit(SERVER_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR) {
    fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
    exit(SERVER_FAILURE);
  }

  /* Parse and set command line arguments */
  while ((option_char = getopt_long(argc, argv, "x:n:ls:p:t:z:h", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(__LINE__);
      case 'n': // segment count
        nsegments = atoi(optarg);
        break;   
      case 'p': // listen-port
        port = atoi(optarg);
        break;
      case 's': // file-path
        server = optarg;
        break;                                          
      case 't': // thread-count
        nworkerthreads = atoi(optarg);
        break;
      case 'z': // segment size
        segsize = atoi(optarg);
        break;
      case 'l': // not used
      case 'x': // not used
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }

  if (!server) {
    fprintf(stderr, "Invalid (null) server name\n");
    exit(__LINE__);
  }

  if (segsize < 128) {
    fprintf(stderr, "Invalid segment size\n");
    exit(__LINE__);
  }

  if (nsegments < 1) {
    fprintf(stderr, "Must have a positive number of segments\n");
    exit(__LINE__);
  }

 if (port < 1024) {
    fprintf(stderr, "Invalid port number\n");
    exit(__LINE__);
  }

  if ((nworkerthreads < 1) || (nworkerthreads > 512)) {
    fprintf(stderr, "Invalid number of worker threads\n");
    exit(__LINE__);
  }


  /* This is where you initialize your shared memory */ 

  /* This is where you initialize the server struct */
  gfserver_init(&gfs, nworkerthreads);

  /* This is where you set the options for the server */
  gfserver_setopt(&gfs, GFS_PORT, port);
  gfserver_setopt(&gfs, GFS_MAXNPENDING, 54);
  gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_cache);
  for(i = 0; i < nworkerthreads; i++) {
    gfserver_setopt(&gfs, GFS_WORKER_ARG, i, "data");
  }
  
  /* This is where you invoke the framework to run the server */
  /* Note that it loops forever */
  gfserver_serve(&gfs);
}
