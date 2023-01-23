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

  // client 
  int client_socket_fd; // file descriptor of the client socket
  struct sockaddr_in client_address;  // struct containing all the information we need about the address of the client socket 
  unsigned int client_len;  // size of the client address

  // server
	struct sockaddr_in server_address;  // struct containing all the information we need about the address of the server socket
	memset(&server_address, 0, sizeof(server_address)); // make sure the struct is empty 
	server_address.sin_family = AF_INET;  // don't care IPv4 or IPv6
	server_address.sin_port = htons(portno); // convert multi-byte integer from host to network byte order (short)
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // exact same thing as htons (but network long this time)
  int yes = 1;  // For setsockopt() SO_REUSEADDR, to avoid errors when a port is still being used by another socket

  // create server socket that will accept incoming connections
	int server_socket_fd; // file descriptor of the server socket
	if ((server_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		exit(1); // error checking: failed to create socket

  // line of code to lose the pesky "address already in use" error message (set SO_REUSEADDR on a socket to true (1))
  setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  // before we start listen()ing, we assign a local socket address to the socket we have just created
  if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		exit(1); // error checking: failed to bind socket (assign an address to it)

  // start listening
	if (listen(server_socket_fd, WAITING_QUEUE_SIZE) < 0)
		exit(1); // error checking: failed to make socket start listening

	// infinite loop: echo server should not terminate after sending its first response; rather, it should prepare to serve another request
	while (1) {
    // get the size of the client parameter
    client_len = sizeof(client_address);

    // connect to the client
    if ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_len)) < 0)
      exit(1); // error checking: failed to accept connection 

    char buffer[BUFSIZE]; // buffer that contains the data we have received
    int recv_msg_len;     // number of bytes of the data we have received

    // receive the message from the client 
    if ((recv_msg_len = recv(client_socket_fd, buffer, BUFSIZE, 0)) <= 0)
      exit(1); // error checking: failed to receive message from the client

    // send the exact same message to the client 
    if (send(client_socket_fd, buffer, recv_msg_len, 0) != recv_msg_len)
      exit(1); // error checking: failed to resend message to the client
  }
	return 0;
}
