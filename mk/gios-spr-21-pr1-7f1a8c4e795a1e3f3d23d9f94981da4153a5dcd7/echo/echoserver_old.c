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

#define BUFSIZE 16

#define USAGE                                                        \
  "usage:\n"                                                         \
  "  echoserver [options]\n"                                         \
  "options:\n"                                                       \
  "  -p                  Port (Default: 30605)\n"                    \
  "  -m                  Maximum pending connections (default: 1)\n" \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"port", required_argument, NULL, 'p'},
    {"maxnpending", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  int option_char;
  int portno = 30605; /* port to listen on */
  int maxnpending = 1;

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      default:
        fprintf(stderr, "%s ", USAGE);
        exit(1);
      case 'm':  // server
        maxnpending = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s ", USAGE);
        exit(0);
        break;
    }
  }

  setbuf(stdout, NULL);  // disable buffering

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }
  if (maxnpending < 1) {
    fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__,
            maxnpending);
    exit(1);
  }

  /* Socket Code Here */
  struct sockaddr_in echoServer, echoClient;
  socklen_t addr_size = sizeof echoClient;
  int sockfd, new_fd;
  char data[BUFSIZE];

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed: ");
    exit(-1);
  }
  
  echoServer.sin_family = AF_INET;
  echoServer.sin_port = htons(portno);
  echoServer.sin_addr.s_addr = INADDR_ANY;
  memset(echoServer.sin_zero, '\0', sizeof(echoServer.sin_zero));

  if (bind(sockfd, (struct sockaddr *)&echoServer, sizeof(echoServer)) < 0) {
    perror("bind() failed: ");
    exit(-1);
  }

  if (listen(sockfd, maxnpending) < 0) {
    perror("listen() failed: ");
    exit(-1);
  }

  while (1) {
    if((new_fd = accept(sockfd, (struct sockaddr *)&echoClient, &addr_size)) < 0) {
      perror("accept() failed: ");
      exit(-1);
    }

    int data_len = 1;

    while (data_len)
    {
      data_len = recv(new_fd, data, BUFSIZE, 0);

      if (data_len) {
        send(new_fd, data, data_len, 0);
        data[data_len] = '\0';
        printf(data);
      }
    }    

    close(new_fd);

  }

  close(sockfd);

}