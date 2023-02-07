#include "gfserver.h"

#define USAGE                                                                         \
"usage:\n"                                                                            \
"  webproxy [options]\n"                                                              \
"options:\n"                                                                          \
"  -p [listen_port]    Listen port (Default: 20121)\n"                                \
"  -s [server]         The server to connect to (Default: GitHub test data)\n"    \
"  -t [thread_count]   Num worker threads (Default is 9, Range is 1-520)\n"               \
"  -h                  Show this help message\n"


/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"help",          no_argument,            NULL,           'h'},
  {"port",          required_argument,      NULL,           'p'},
  {"server",        required_argument,      NULL,           's'},
  {"thread-count",  required_argument,      NULL,           't'},
  {NULL,            0,                      NULL,            0}
};

extern ssize_t handle_with_file(gfcontext_t *ctx, const char *path, void* arg);
extern ssize_t handle_with_curl(gfcontext_t *ctx, const char *path, void* arg);

static gfserver_t gfs;

static void _sig_handler(int signo){
  if (signo == SIGTERM || signo == SIGINT){
    gfserver_stop(&gfs);
    exit(signo);
  }
}

int main(int argc, char **argv) {
  int i;
  int option_char = 0;
  unsigned short port = 20121;
  unsigned short nworkerthreads = 9;
  const char *server = "https://raw.githubusercontent.com/gt-cs6200/image_data";

  // disable buffering on stdout so it prints immediately 
  setbuf(stdout, NULL);

  if (signal(SIGINT, _sig_handler) == SIG_ERR){
    fprintf(stderr,"Can't catch SIGINT...exiting.\n");
    exit(SERVER_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR){
    fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
    exit(SERVER_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "qp:s:t:xh", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'x':
      case 'q':
      	break;
      case 'p': // listen-port
        port = atoi(optarg);
        break;
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 't': // thread-count
        nworkerthreads = atoi(optarg);
        break;
      case 's': // file-path
        server = optarg;
        break;              
    }
  }


  if (port < 1024) {
    fprintf(stderr, "Invalid port number\n");
    exit(__LINE__);
  }

  if (NULL == server) {
    fprintf(stderr, "Invalid (null) server name\n");
    exit(__LINE__);
  }

  if ((nworkerthreads < 1) || (nworkerthreads > 520)) {
    fprintf(stderr, "Invalid number of worker threads\n");
    exit(__LINE__);
  }

  // Initialize server structure here
  gfserver_init(&gfs, nworkerthreads);

  // Set server options here
  gfserver_setopt(&gfs, GFS_MAXNPENDING, 321);
  gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_curl);
  gfserver_setopt(&gfs, GFS_PORT, port);

  // Set up arguments for worker here
  for(i = 0; i < nworkerthreads; i++) {
    gfserver_setopt(&gfs, GFS_WORKER_ARG, i, server);
  }
  
  // Invoke the framework - this is an infinite loop and shouldn't return
  gfserver_serve(&gfs);

  // not reached
  return -1;

}