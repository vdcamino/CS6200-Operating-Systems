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

/* A buffer large enough to contain the longest allowed string */
#define BUFSIZE 2012

#define USAGE                                                          \
  "usage:\n"                                                           \
  "  echoclient [options]\n"                                           \
  "options:\n"                                                         \
  "  -s                  Server (Default: localhost)\n"                \
  "  -p                  Port (Default: 20121)\n"                      \
  "  -m                  Message to send to server (Default: \"Hello " \
  "world.\")\n"                                                        \
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
  unsigned short portno = 20121;
  char *message = "Hello World!!";

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

  /*****************************/
    // REFERENCES TAKEN FROM 
    // https://beej.us/guide/bgnet/examples/server.c
    // https://beej.us/guide/bgnet/examples/Client.c
    //https://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
    // https://man7.org/linux/man-pages/dir_all_alphabetic.html
  /*****************************/
  

    int csock_fd,bytes,status;
    
    struct addrinfo specs,*addrs,*temp;

    // clearing all the previous values if any
    memset(&specs,0,sizeof(specs));
    specs.ai_family= AF_UNSPEC; // ipv4 or ipv6
    specs.ai_socktype=SOCK_STREAM;
    char port[10];
    snprintf (port, sizeof(port), "%d",portno);
   // printf("port--> %s",port);
    status= getaddrinfo(hostname,port,&specs,&addrs);
    if(status!=0)
    {
      perror("getaddrinfo failed:\n");
    }

    for(temp=addrs;temp!=NULL;temp=temp->ai_next)
    {
      csock_fd=socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol);
      if((csock_fd)==-1)
      {
        perror("Socket creation failed:");
        continue;
      }
      
      if(connect(csock_fd,temp->ai_addr,temp->ai_addrlen)==-1)
      {
       // perror("Connection failed:");
        close(csock_fd);
        continue;
      }
      break;
    }

    if(temp==NULL)
    {
      perror("Null addrinfo:");
      return 2;
    }

    // freeing the addrinfo once connection successful
    freeaddrinfo(addrs);
    
    // Getting ready to send message to server
    if(send(csock_fd,message,strlen(message),0)==-1)
    {
      perror("Incomplete transfer of data:");
      exit(1);
    }
    int length=16;
    char *buffer=malloc(length);
    if ((bytes = recv(csock_fd, buffer, 15, 0)) == -1)
          //printf("Bytes received--%d",bytes);
              {
                 exit(1);
              }

          buffer[bytes] = '\0'; // null-terminated the string message so as to print it.

          printf("%s",buffer);
   
    close(csock_fd);
  free(buffer);
}
