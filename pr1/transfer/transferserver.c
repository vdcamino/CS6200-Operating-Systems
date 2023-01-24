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

  // client 
  int client_socket_fd; // file descriptor of the client socket
  struct sockaddr_in client_address;  // struct containing all the information we need about the address of the client socket 
  
  // server
	struct sockaddr_in server_address;  // structs containing all the information we need about the address of the server socket
  socklen_t addr_len = sizeof server_address;
	memset(&server_address, 0, sizeof(server_address)); // make sure the struct is empty 
	server_address.sin_port = htons(portno); // convert multi-byte integer from host to network byte order (short)
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // exact same thing as htons (but network long this time)
  int yes = 1;  // For setsockopt() SO_REUSEADDR, to avoid errors when a port is still being used by another socket
  int pid; // process id of the child process we will use to actually send the file by chunks
  FILE *fp; // file pointer 
  char buffer[BUFSIZE]; // buffer that contains the data to send

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

  // infinite loop: "Your server should not stop after a single transfer, but should continue accepting and transferring files to other clients"
	while (1) {
    // connect to the client
    if ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &addr_len)) < 0)
      exit(1); // error checking: failed to accept connection 

    int file_to_send, packet_to_send;
    int file_sent = 0;
    int packet_sent = 0;
    // server opens the pre-defined file 
    // file_to_send = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fp = fopen(filename, "r+");
    if (!fp)
      exit(1); // error checking: failed to open the file 

    // if (file_to_send < 0) 
    //   exit(1); // error checking: failed to open the file 

    // send file by chunks, exit while loop when there is no more data to transfer
     
		bzero(buffer, BUFSIZE); // clean buffer

    // READ 7.4 Handling Partial send()s from Beej
    // send by chunks 
    // server should close the socket after finishing sending the data, that's how the client knows the transfer is done **** 24/01/2023

    // int read_len;
    // while ((read_len = fread(buffer, sizeof(char), BUFSIZE, file)) > 0) {
    //   if (send(netsockfd, buffer, read_len, 0) < 0) {
    //     fprintf(stderr, "File failed to send.\n");
    //     fclose(fp);
    //     close(netsockfd);
    //     exit(1);
    //   }
    //   total_size += read_len;
    // }

    // to continue *****
    
          
    
    while(packet_to_send != 0){
      memset(buffer, '\0', BUFSIZE);
      packet_to_send = read(file_to_send, buffer, BUFSIZE - 1); // -1 to avoid overflow, read returns the number of bytes it has read 
      if (packet_to_send < 0) {
        exit(1); // error checking: failed to read the file 
      }
      // send file
      char *cur_position = buffer; // keeps track of where we stopped sending the file 
      while (packet_to_send > 0){ // stop only when you have sent all the bytes from this packet 
        packet_sent = send(client_socket_fd, cur_position, packet_to_send, 0);
        if (packet_sent < 0) 
          exit(1); // error checking: failed to send data
        cur_position += packet_sent;
        packet_to_send -= packet_sent;
      }
    }
    exit(0); // ended the transfer, exit the server so client can know it is finished 

  }
  // close sockets and file 
  close(client_socket_fd);
  close(server_socket_fd);
  fclose(fp);
	return 0;
}