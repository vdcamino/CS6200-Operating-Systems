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
#include <arpa/inet.h>

/* A buffer large enough to contain the longest allowed string */
#define BUFSIZE 16

#define USAGE                                                          \
  "usage:\n"                                                           \
  "  echoclient [options]\n"                                           \
  "options:\n"                                                         \
  "  -s                  Server (Default: localhost)\n"                \
  "  -p                  Port (Default: 30605)\n"                      \
  "  -m                  Message to send to server (Default: \"Hello " \
  "spring.\")\n"                                                       \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
  int option_char = 0;
  char *hostname = "localhost";
  unsigned short portno = 30605;
  char *message = "Hello Spring!!";

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 's':  // server
        hostname = optarg;
        break;
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'm':  // message
        message = optarg;
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
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

  if (NULL == message) {
    fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
    exit(1);
  }

  if (NULL == hostname) {
    fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
    exit(1);
  }

  /* Socket Code Here */
  // Converting portno to char * for getaddrinfo()
  char * port = NULL;
  port = (char*)malloc(10*sizeof(char));
  sprintf(port, "%d", portno);

  // Initializing variables
  struct addrinfo hints, *res;
  struct in6_addr server_addr;
  int ip_version, server_sock_fd, conn, data_len_in;
  char echoMessage[BUFSIZE];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype =  SOCK_STREAM;

  // Using inet_pton() to check if the hostname is IPV6 or IPV4
  // Referenced inet_pton from Beej's guide & man pages to get AI_FAMILY type
  ip_version = inet_pton(AF_INET, hostname, &server_addr);
  if (ip_version == 1) {
      hints.ai_family = AF_INET;
  } else {
      ip_version = inet_pton(AF_INET6, hostname, &server_addr);
      if (ip_version == 1) {
        hints.ai_family = AF_INET6;
      }
  }

  // Using getaddrinfo to populate the addrinfo struct
  if (getaddrinfo(hostname, port, &hints, &res) < 0) {
    perror("getaddrinfo() failed: ");
    exit(1);
  }

  // SOCKET()
  if ((server_sock_fd = socket(res->ai_family, res->ai_socktype, 0)) < 0) {
    perror("socket() failed: ");
    exit(1);
  }

  // CONNECT()
  if ((conn = connect(server_sock_fd, res->ai_addr, res->ai_addrlen)) < 0) {
    perror("connect() failed: ");
    exit(1);
  }

  // SEND()
  if ((send(server_sock_fd, message, strlen(message), 0)) < 0 ) {
    perror("send() failed: ");
    exit(1);
  }

  // RECV()
  if ((data_len_in = recv(server_sock_fd, echoMessage, BUFSIZE, 0)) < 0 ) {
    perror("send() failed: ");
    exit(1);
  }

  echoMessage[data_len_in] = '\0';
  printf("%s\n", echoMessage);

  // Freeing up resources
  free(port);
  freeaddrinfo(res);
  close(server_sock_fd);

  exit(0);
}