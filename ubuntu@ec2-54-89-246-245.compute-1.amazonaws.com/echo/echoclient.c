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

// A buffer large enough to contain the longest allowed string 
#define BUFSIZE 256 // size of the message tranferred to/from the server

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
  while ((option_char =
              getopt_long(argc, argv, "p:s:m:hx", gLongOptions, NULL)) != -1) {
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
  // First test copied from https://gist.github.com/suyash/2488ff6996c98a8ee3a84fe3198a6f85

  struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	// creates binary representation of server name
	// and stores it as sin_addr
	// http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
	inet_pton(AF_INET, hostname, &server_address.sin_addr);

	// htons: port in network order format
	server_address.sin_port = htons(portno);

	// open a stream socket
	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("could not create socket\n");
		return 1;
	}

	// TCP is connection oriented, a reliable connection
	// **must** be established before any data is exchanged
	if (connect(sock, (struct sockaddr*)&server_address,
	            sizeof(server_address)) < 0) {
		printf("could not connect to server\n");
		return 1;
	}

	// send

	// data that will be sent to the server
	const char* data_to_send = "Gangadhar Hi Shaktimaan hai";
	send(sock, data_to_send, strlen(data_to_send), 0);

	// receive

	int n = 0;
	int len = 0, maxlen = 100;
	char buffer[maxlen];
	char* pbuffer = buffer;

	// will remain open until the server terminates the connection
	while ((n = recv(sock, pbuffer, maxlen, 0)) > 0) {
		pbuffer += n;
		maxlen -= n;
		len += n;

		buffer[len] = '\0';
		printf("received: '%s'\n", buffer);
	}

	// close the socket
	close(sock);
	return 0;
}
