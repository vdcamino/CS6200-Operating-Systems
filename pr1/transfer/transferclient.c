#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h> // file control options 
#include <arpa/inet.h> // inet functions

#define BUFSIZE 512

#define USAGE                                                \
  "usage:\n"                                                 \
  "  transferclient [options]\n"                             \
  "options:\n"                                               \
  "  -h                  Show this help message\n"           \
  "  -p                  Port (Default: 10823)\n"            \
  "  -s                  Server (Default: localhost)\n"      \
  "  -o                  Output file (Default cs6200.txt)\n" 

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {{"server", required_argument, NULL, 's'},
                                       {"output", required_argument, NULL, 'o'},
                                       {"port", required_argument, NULL, 'p'},
                                       {"help", no_argument, NULL, 'h'},
                                       {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
  unsigned short portno = 10823;
  int option_char = 0;
  char *hostname = "localhost";
  char *filename = "cs6200.txt";

  setbuf(stdout, NULL);

  /* Parse and set command line arguments */ 
  while ((option_char = getopt_long(argc, argv, "xp:s:h:o:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
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
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'o':  // filename
        filename = optarg;
        break;
    }
  }

  if (NULL == hostname) {
    fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
    exit(1);
  }

  if (NULL == filename) {
    fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
    exit(1);
  }

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  // Socket Code Here 
  
  // create server socket
  struct sockaddr_in server_address;                  // struct containing all the information we need about the address of the server socket 
	memset(&server_address, 0, sizeof(server_address)); // make sure the struct is empty 

	// create binary representation of server name and stores it as sin_addr. pton = "printable to network"
	inet_pton(AF_INET, hostname, &server_address.sin_addr);
	server_address.sin_port = htons(portno); // convert multi-byte integer from host to network byte order (short)

	// create client socket
	int client_socket_fd, file_recv;
	if ((client_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    exit(1); // error checking: failed to create socket

	// establish connection before initiating data exchange
	if (connect(client_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) 
	  exit(1); // error checking: failed to connect to server 

  // open file to start writing on it 
  file_recv = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (file_recv < 0)
    exit(1); // error checking: failed to open file

  // receive the file
  char buffer[BUFSIZE]; // buffer that contains the data to send 
  int file_packet_size;
  int total_file_size = 0;
  // loop to receive file by chunks
  while((file_packet_size = recv(client_socket_fd, buffer, BUFSIZE, 0)) > 0){
    memset(buffer, '\0', BUFSIZE);
    total_file_size += file_packet_size;
    if (file_packet_size < 0)
      exit(1); // error checking: failed to receive data
    if(write(file_recv, buffer, file_packet_size) < file_packet_size)
      exit(1); // error checking: failed to write file
  }

  close(file_recv);
  close(client_socket_fd);

  return 0;
}
