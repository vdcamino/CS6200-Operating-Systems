#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>

#define BUFSIZE 5041

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 50419)\n"                                \
"  -m                  Maximum pending connections (default: 1)\n"            \
"  -h                  Show this help message\n"                              \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"maxnpending",   required_argument,      NULL,           'm'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};

void dostuff(int); /* function prototype */
int main(int argc, char **argv) {
  int option_char;
  int portno = 50419; /* port to listen on */
  int maxnpending = 1;

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1) {
   switch (option_char) {
      case 'p': // listen-port
        portno = atoi(optarg);
        break;
      default:
        fprintf(stderr, "%s ", USAGE);
        exit(1);
      case 'm': // server
        maxnpending = atoi(optarg);
        break;
      case 'h': // help
        fprintf(stdout, "%s ", USAGE);
        exit(0);
        break;
    }
  }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    if (maxnpending < 1) {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }


  /* Socket Code Here */
  int sockfd, newsockfd, clilen, pid;
  struct sockaddr_in serv_addr, cli_addr;
  int serv_addrSize=sizeof(struct sockaddr_in);
  /* open a new socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0){
    fprintf(stderr, "ERROR opening socket");
    exit(1);
  }
  // enable SO_REUSEADDR option, solved #2 failed test
  int enable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    fprintf(stderr,"setsockopt(SO_REUSEADDR) failed");
    exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    fprintf(stderr, "\nERROR on binding\n");
    exit(1);
  }
  getsockname( sockfd, (struct sockaddr *) &serv_addr,(socklen_t *)&serv_addrSize);
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  while(1){
    /* establish a connection with the client*/
    newsockfd = accept(sockfd, (struct sockaddr *) & cli_addr, (socklen_t *)&clilen);
    if (newsockfd < 0){
      fprintf(stderr, "\nERROR on accept\n");
      exit(1);
    }
    /* create a new process*/
    pid = fork();
    if(pid<0){
      fprintf(stderr, "\nERROR on fork\n");
      exit(1);
    }
    if(pid==0){
      close(sockfd);
      dostuff(newsockfd);
      exit(0);
    }
    else
      close(newsockfd);
  }
  return 0;
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int received, sent;
   char buffer[BUFSIZE];

   bzero(buffer,BUFSIZE);
   //n = read(sock,buffer,BUFSIZE-1);
   received = recv(sock, buffer, BUFSIZE, 0);
   if (received < 0){
     fprintf(stderr, "\nERROR reading from socket\n");
     exit(1);
   }
   printf("%s",buffer);
   printf("\n");
   //n = write(sock, buffer, strlen(buffer));
   sent = send(sock, buffer, BUFSIZE, 0);
   if (sent < 0){
     fprintf(stderr, "\nERROR writing to socket\n");
     exit(1);
   }
}
