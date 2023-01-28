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
  // create and open file to start writing on it 
  int file_recv = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (file_recv < 0)
    exit(11); // error checking: failed to open file

  // create client socket
	int client_socket_fd;
	if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    exit(14); // error checking: failed to create socket

  // initialize sockaddr_in
  struct sockaddr_in server_address;
  struct hostent *server = gethostbyname(hostname);
  bzero((char*) &server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(portno);
  bcopy((char*)server->h_addr, (char*)&server_address.sin_addr.s_addr, server->h_length);

  // establish connection before initiating data exchange
	if (connect(client_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) 
	  exit(12); // error checking: failed to connect to server 

  // receive the file
  char buffer[BUFSIZE]; // buffer that contains the data to send 
  memset(buffer, '\0', BUFSIZE);
  int bytes_rcvd = 0;
  // loop to receive file by chunks
  while((bytes_rcvd = recv(client_socket_fd, buffer, BUFSIZE, 0)) > 0){
    write(file_recv, buffer, bytes_rcvd);
    memset(buffer, '\0', BUFSIZE);
  }

  // free resources 
  close(file_recv);
  close(client_socket_fd);
  return 0;
}
