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
  // Initializing variables
  struct sockaddr_in6 echoServer, echoClient;
  socklen_t addr_size = sizeof echoClient;
  int server_sock_fd, client_sock_fd, data_len_in, optval = 1;
  char data[BUFSIZE];

  // SOCKET()
  if ((server_sock_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed: ");
    exit(1);
  }

  // Setsockopt() to reuse address and avoid "Address already in use" issue
  // Found in Beej's networking guide for resolving this issue
  if ((setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval)) < 0) {
    perror("setsockopt() failed: ");
    exit(-1);
  }

  // Populating server sockaddr_in struct
  memset(&echoServer, 0, sizeof(echoServer));
  echoServer.sin6_family = AF_INET6;
  echoServer.sin6_port = htons(portno);
  echoServer.sin6_addr = in6addr_any;

  // BIND()
  if (bind(server_sock_fd, (struct sockaddr *)&echoServer, sizeof(echoServer)) < 0) {
    perror("bind() failed: ");
    exit(1);
  }

  // LISTEN()
  if (listen(server_sock_fd, maxnpending) < 0) {
    perror("listen() failed: ");
    exit(1);
  }

  // Keeping Server connection open - Not sure if efficient/safe
  while (1) {
    // ACCEPT()
    if((client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&echoClient, &addr_size)) < 0) {
      perror("accept() failed: ");
      exit(1);
    }

    // RECEIVE()
    if ((data_len_in = recv(client_sock_fd, data, BUFSIZE, 0)) <0 ) {
      perror("recv() failed: ");
      exit(1);
    }

    data[data_len_in] = '\0';

    // SEND()
    if ((send(client_sock_fd, data, strlen(data), 0)) < 0 ) {
      perror("send() failed: ");
      exit(1);
    }

    close(client_sock_fd);

  }

  close(server_sock_fd);
  exit(0);

}