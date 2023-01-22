#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <getopt.h>
#include <netdb.h>
#include <arpa/inet.h> // inet_ntoa

#define BUFSIZE 256
#define WAITING_QUEUE_SIZE 10 // number of clients allowed in the waiting queue

#define USAGE                                                        \
  "usage:\n"                                                         \
  "  echoserver [options]\n"                                         \
  "options:\n"                                                       \
  "  -p                  Port (Default: 10823)\n"                    \
  "  -m                  Maximum pending connections (default: 5)\n" \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"maxnpending", required_argument, NULL, 'm'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv) {
  int maxnpending = 5;
  int option_char;
  int portno = 10823; /* port to listen on */

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "hx:m:p:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'm':  // server
        maxnpending = atoi(optarg);
        break;
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s ", USAGE);
        exit(0);
        break;
      default:
        fprintf(stderr, "%s ", USAGE);
        exit(1);
    }
  }

  setbuf(stdout, NULL);  // disable buffering

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
    exit(1);
  }
  if (maxnpending < 1) {
    fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
    exit(1);
  }

  // Socket Code Here

	struct sockaddr_in server_address; // struct containing all the information we need about the address of the server socket 
	memset(&server_address, 0, sizeof(server_address)); // make sure the struct is empty 
	server_address.sin_family = AF_INET;  // don't care IPv4 or IPv6
	server_address.sin_port = htons(portno); // convert multi-byte integer from host to network byte order (short)
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // exact same thing as htons (but network long this time)

  // create server socket
	int socket_fd; // declare file descriptor (the server socket)
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		exit(1); // error checking: failed to create socket

  // before we start listen()ing, we assign a local socket address to the socket we have just created
  if ((bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address))) < 0)
		exit(1); // error checking: failed to bind socket (assign an address to it)

	if (listen(socket_fd, WAITING_QUEUE_SIZE) < 0)
		exit(1); // error checking: failed to make socket start listening

	// socket address used to store client address
	struct sockaddr_in client_address;
	socklen_t client_address_len = 0;

	// infinite loop: echoserver should not terminate after sending its first response; rather, it should prepare to serve another request
	while (1) {
		// create new socket to accept the incoming connection
		int new_socket_fd;
		if ((new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_len)) < 0)
			exit(1); // error checking: failed to create socket that would accept new data 

    int res = 0, len = 0, maxlen = BUFSIZE;
    char buffer[maxlen];
		char *pbuffer = buffer;

		// keep running as long as the client keeps the connection open
    // 0 from recv means connection closed 
		while ((res = recv(new_socket_fd, pbuffer, maxlen, 0)) > 0) {
      pbuffer += res;
			len += res;
      maxlen -= res;
      buffer[len] = '\0'; // last byte + 1 is null
			// echo server sends content back
			send(new_socket_fd, buffer, len, 0);
		}

		close(new_socket_fd);
	}

	close(socket_fd);
	return 0;
}
