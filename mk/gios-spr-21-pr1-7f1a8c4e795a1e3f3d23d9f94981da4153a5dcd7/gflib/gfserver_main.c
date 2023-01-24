#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "content.h"
#include "gfserver-student.h"
#include "gfserver.h"

#define USAGE                                                          \
  "usage:\n"                                                           \
  "  gfserver_main [options]\n"                                        \
  "options:\n"                                                         \
  "  -p [listen_port]    Listen port (Default: 30605)\n"               \
  "  -m [content_file]   Content file mapping keys to content files\n" \
  "  -h                  Show this help message.\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"port", required_argument, NULL, 'p'},
    {"content", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
  char *content_map_file = "content.txt";
  unsigned short port = 30605;
  int option_char = 0;
  gfserver_t *gfs;

  setbuf(stdout, NULL);  // disable caching

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "hrt:m:p:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'p':  // listen-port
        port = atoi(optarg);
        break;
      case 'm':  // file-path
        content_map_file = optarg;
        break;
    }
  }

  content_init(content_map_file);

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 521);

  /* this implementation does not pass any extra state, so it uses NULL. */
  /* this value could be non-NULL.  You might want to test that in your own
   * code. */
  gfserver_set_handlerarg(&gfs, NULL);

  /*Loops forever*/
  gfserver_serve(&gfs);
}
