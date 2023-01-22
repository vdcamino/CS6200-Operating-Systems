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

/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;
  fwrite(data, 1, data_len, file);
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  char *server = "localhost";
  char *workload_path = "workload.txt";
  char *req_path = NULL;
  int option_char = 0;
  unsigned short port = 10880;


  char local_path[PATH_BUFFER_SIZE];
  int returncode = 0;
  int nthreads = 16;
  int nrequests = 12;

  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;

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

  /* Build your queue of requests here */
  for (int i = 0; i < nrequests; i++) {
    /* Note that when you have a worker thread pool, you will need to move this
     * logic into the worker threads */
    req_path = workload_get_path();

    if (strlen(req_path) > PATH_BUFFER_SIZE) {
      fprintf(stderr, "Request path exceeded maximum of %d characters\n.", PATH_BUFFER_SIZE);
      exit(EXIT_FAILURE);
    }

    localPath(req_path, local_path);

    file = openFile(local_path);

    gfr = gfc_create();
    gfc_set_path(&gfr, req_path);

    gfc_set_port(&gfr, port);
    gfc_set_server(&gfr, server);
    gfc_set_writearg(&gfr, file);
    gfc_set_writefunc(&gfr, writecb);



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
      if (0 > unlink(local_path)) {
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
      }
    }

    fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
    fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
            gfc_get_filelen(&gfr));

    gfc_cleanup(&gfr);

    /*
     * note that when you move the above logic into your worker thread, you will
     * need to coordinate with the boss thread here to effect a clean shutdown.
     */
  }

  gfc_global_cleanup();  /* use for any global cleanup for AFTER your thread
                          pool has terminated. */

  return 0;
}
