#include "gfclient-student.h"
#include "gfclient.h"
#include "workload.h"

#define BUFSIZE 630
#define PATH_BUFFER_SIZE 2080

#define USAGE                                                         \
  "usage:\n"                                                          \
  "  webclient [options]\n"                                           \
  "options:\n"                                                        \
  "  -h                  Show this help message\n"                    \
  "  -n [num_requests]   Requests download per thread (Default: 2)\n" \
  "  -p [server_port]    Server port (Default: 30605)\n"              \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n"       \
  "  -l [workload_path]  Path to workload file (Default: workload.txt)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"nrequests", required_argument, NULL, 'n'},
    {"port", required_argument, NULL, 'p'},
    {"server", required_argument, NULL, 's'},
    {"workload-path", required_argument, NULL, 'l'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

static void Usage() { fprintf(stdout, "%s", USAGE); }

static void localPath(char *req_path, char *local_path) {
  static int counter = 0;

  snprintf(local_path, PATH_BUFFER_SIZE, "%s_%06d", &req_path[1], counter++);
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
  unsigned short port = 30605;
  char *workload_path = "workload.txt";
  int option_char = 0;
  int nrequests = 5;
  int returncode;
  gfcrequest_t *gfr;
  FILE *file;
  char *req_path;
  char local_path[PATH_BUFFER_SIZE];
  int i;

  setbuf(stdout, NULL);  // disable buffering

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "l:rhp:s:n:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'h':  // help
        Usage();
        exit(0);
        break;
      case 's':  // server
        server = optarg;
        break;
      case 'p':  // port
        port = atoi(optarg);
        break;
      default:
        Usage();
        exit(1);
      case 'n':  // nrequests
        nrequests = atoi(optarg);
        break;
      case 'l':  // workload-path
        workload_path = optarg;
        break;
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  gfc_global_init();

  /*Making the requests...*/
  for (i = 0; i < nrequests; i++) {
    req_path = workload_get_path();

    if (strlen(req_path) > 256) {
      fprintf(stderr, "Request path exceeded maximum of 256 characters\n.");
      exit(EXIT_FAILURE);
    }

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

    fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
    fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
            gfc_get_filelen(&gfr));

    gfc_cleanup(&gfr);
  }

  gfc_global_cleanup();

  // workload_destroy();  // clean up workload package

  return 0;
}
