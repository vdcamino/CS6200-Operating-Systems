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
#include <fcntl.h> // file control options 

#define BUFSIZE 512
#define WAITING_QUEUE_SIZE 10 // number of clients allowed in the waiting queue

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

// beej's sendall function adapted
void send_file(int socket_fd, int file_to_send){
  char buf[BUFSIZE]; // buffer used to read and send data
  int bytes_sent = 0; // amount of bytes we have alreasy sent 
  int bytes_left = 0; // amount of bytes we stil have to send
  int packet_size; // amount of bytes we have sent during a call of send()
  int bytes_to_send;
  do{
    bytes_to_send = read(file_to_send, buf, BUFSIZE - 1);
    if (bytes_to_send < 0)
      exit(1); // error checking: failed to read the file
    bytes_left = bytes_to_send;
    while (bytes_left){
      packet_size = send(socket_fd, buf + bytes_sent, bytes_left, 0);
      if (packet_size == -1)
        exit(1); // error checking: failed to send the file packet 
      bytes_sent += packet_size;
      bytes_left -= packet_size;
    }
  } while(bytes_to_send);
} 

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

  // server
  struct sockaddr_in server_address, client_address;  // structs containing all the information we need about the address of the server socket
	memset(&server_address, 0, sizeof(server_address)); // make sure the struct is empty 
	server_address.sin_port = htons(portno); // convert multi-byte integer from host to network byte order (short)
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // exact same thing as htons (but network long this time)
  server_address.sin_family = AF_INET;
  int yes = 1;  // For setsockopt() SO_REUSEADDR, to avoid errors when a port is still being used by another socket
  int file_to_send; 
  int pid;
  int client_len = sizeof(client_address);  // size of the client address
  int server_len = sizeof(server_address);

  // create server socket that will accept incoming connections
	int server_socket_fd; // file descriptor of the server socket
	if ((server_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		exit(1); // error checking: failed to create socket

  // line of code to lose the pesky "address already in use" error message (set SO_REUSEADDR on a socket to true (1))
  setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  // before we start listen()ing, we assign a local socket address to the socket we have just created
  if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		exit(1); // error checking: failed to bind socket (assign an address to it)

  getsockname(server_socket_fd, (struct sockaddr *) &server_address,(socklen_t *)&server_len);

  // start listening
	if (listen(server_socket_fd, WAITING_QUEUE_SIZE) < 0)
		exit(1); // error checking: failed to make socket start listening
  
  // infinite loop: "Your server should not stop after a single transfer, but should continue accepting and transferring files to other clients"
	while (1){
    // create connection 
    int connection_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_len);
    if (connection_socket_fd < 0)
      exit(1); // error checking: failed to accept connection 
    pid = fork();
    if(pid < 0){
      exit(1); // error checking: failed to fork
    }
    if(pid == 0){ // success
      close(server_socket_fd);
      // server opens the pre-defined file 
      file_to_send = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      if (file_to_send < 0)
        exit(1); // error checking: failed to open the file 
      // send file 
      send_file(connection_socket_fd, file_to_send);
      exit(0);
    } else {
      // file sent, close socket connection
      // sleep(1); // do not close the connection immediately after sending the file
      close(connection_socket_fd);
    }
  }
	return 0;
}