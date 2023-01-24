#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_pton

#define BUFSIZE 256

#define USAGE                                                          \
  "usage:\n"                                                           \
  "  echoclient [options]\n"                                           \
  "options:\n"                                                         \
  "  -s                  Server (Default: localhost)\n"                \
  "  -p                  Port (Default: 10823)\n"                      \
  "  -m                  Message to send to server (Default: \"Hello " \
  "Summer.\")\n"                                                       \
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
  unsigned short portno = 10823;
  int option_char = 0;
  char *message = "Hello Summer!!";
  char *hostname = "localhost";

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:s:m:hx", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 's':  // server
        hostname = optarg;
        break;
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 'm':  // message
        message = optarg;
        break;
    }
  }

  setbuf(stdout, NULL);  // disable buffering

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
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

  // create server socket
  struct sockaddr_in server_address;                  // struct containing all the information we need about the address of the server socket 
	memset(&server_address, 0, sizeof(server_address)); // make sure the struct is empty 
	server_address.sin_family = AF_INET;                // don't care if it is IPv4 or IPv6

	// create binary representation of server name and stores it as sin_addr. pton = "printable to network"
	inet_pton(AF_INET, hostname, &server_address.sin_addr);
	server_address.sin_port = htons(portno); // convert multi-byte integer from host to network byte order (short)

	// create client socket
	int client_socket_fd;
	if ((client_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    exit(1); // error checking: failed to create socket

	// establish connection before initiating data exchange
	if (connect(client_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) 
	  exit(1); // error checking: failed to connect to server 

  // send message to the server
  send(client_socket_fd, message, strlen(message), 0);

  // receive message from the server
  char buffer[BUFSIZE];
  int bytes_rcvd = 0;         // number of bytes read during a single recv() 
  if ((bytes_rcvd = recv(client_socket_fd, buffer, BUFSIZE - 1, 0)) <= 0)
    exit(1);                  // error checking: failed to receive messsage back 
  buffer[bytes_rcvd] = '\0';  // according to the project instructions: "Neither your server nor your client should assume that the string response will be null-terminated"
  printf("%s", buffer);       // data that we have received from the server

	// close the client socket
	close(client_socket_fd);
	return 0;
}
