#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 512

#define USAGE                                            \
  "usage:\n"                                             \
  "  transferserver [options]\n"                         \
  "options:\n"                                           \
  "  -h                  Show this help message\n"       \
  "  -f                  Filename (Default: 6200.txt)\n" \
  "  -p                  Port (Default: 10823)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"filename", required_argument, NULL, 'f'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  char *filename = "6200.txt"; // file to transfer 
  int portno = 10823;          // port to listen on 
  int option_char;

  setbuf(stdout, NULL);  // disable buffering

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "hf:xp:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'f':  // file to transfer
        filename = optarg;
        break;
    }
  }

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  if (NULL == filename) {
    fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
    exit(1);
  }

  /* Socket Code Here */
}
