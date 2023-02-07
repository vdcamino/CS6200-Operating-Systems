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

#define BUFSIZE 2012

#define USAGE                                                        \
  "usage:\n"                                                         \
  "  echoserver [options]\n"                                         \
  "options:\n"                                                       \
  "  -p                  Port (Default: 20121)\n"                    \
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
  int portno = 20121; /* port to listen on */
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
     
     /*****************************/
    // REFERENCES TAKEN FROM 
    // https://beej.us/guide/bgnet/examples/server.c
    // https://beej.us/guide/bgnet/examples/Client.c
    //https://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
    // https://man7.org/linux/man-pages/dir_all_alphabetic.html
    /*****************************/

     // create socket fd for server and client

     int ssock_fd,cli_fd ,status,bytes;
     
     
     struct addrinfo specs,*addrs,*temp;
     struct sockaddr_storage client_address;
     socklen_t addr_size;
     int option=1;

      memset(&specs,0,sizeof(specs)) ; // pre-setting all the values to 0
     specs.ai_family=AF_UNSPEC; // ip-agnostic
     specs.ai_socktype=SOCK_STREAM; 
     specs.ai_flags=AI_PASSIVE;

      char port[10];// convert the unsigned short int to string
    snprintf (port, sizeof(port), "%d",portno);
     status = getaddrinfo(NULL,port,&specs,&addrs);//
     if(status==-1){
       perror("getaddrinfo error:");
     }

    // Looping through the addrinfos fetched by getaddrinfo
    for(temp=addrs;temp!=NULL;temp=temp->ai_next)
    {       //socket creation
            ssock_fd= socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol);
          if(ssock_fd==-1){
            perror("Socket error:");
            continue;
          }
          // setting the socket option to resuse address
          if(setsockopt(ssock_fd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(int))<0)
          {
            perror("Socket resuse:");
            exit(1);
            continue;
          }
          //binding the socket
          if(bind(ssock_fd,temp->ai_addr,temp->ai_addrlen)==-1)
          {
            close(ssock_fd);
            perror("Binding error:");
            continue;
          }
          break;
    }

    freeaddrinfo(addrs);

    if(temp == NULL)
    {
     fprintf(stderr, "Binding failed\n");
      exit(1);
    }

    if(listen(ssock_fd,maxnpending)==-1)
    {
      perror("Listening error");
      exit(1);
    }
    
     int length=16;
        char *buffer=malloc(length);
        
     while(1)
     {
        addr_size=sizeof(client_address);
        cli_fd=accept(ssock_fd,(struct sockaddr*)&client_address,&addr_size);
        if(cli_fd==-1)
        {
          perror("Accept failed:");
          continue;
        }
        
       
        if ((bytes = recv(cli_fd, buffer, 15, 0)) == -1)
          //printf("Bytes received--%d",bytes);
              {
                 exit(1);
              }

          buffer[bytes] = '\0'; // null-terminated the string message so as to print it.

          printf("%s",buffer);
   
        if(send(cli_fd,buffer,15,0)==-1)
              {
                perror("Incomplete transfer of data:");
                exit(1);
              }
       close(cli_fd);
              free(buffer);
     }
    
}
       
