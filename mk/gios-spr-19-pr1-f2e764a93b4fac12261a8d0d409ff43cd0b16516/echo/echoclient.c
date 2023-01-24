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

/* Be prepared accept a response of this length */
#define BUFSIZE 5041

#define USAGE                                                                       \
    "usage:\n"                                                                      \
    "  echoclient [options]\n"                                                      \
    "options:\n"                                                                    \
    "  -s                  Server (Default: localhost)\n"                           \
    "  -p                  Port (Default: 50419)\n"                                  \
    "  -m                  Message to send to server (Default: \"hello world.\")\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 50419;
    char *message = "Hello World!!";

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'm': // message
            message = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == message)
    {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    int sockfd, sent, received;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFSIZE];
    /* Open a new socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
      fprintf(stderr, "ERROR opening socket\n");
      exit(1);
    }
    /* Initialize server info*/
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Establish a connection to the server*/
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
      fprintf(stderr, "ERROR connecting\n");
      exit(1);
    }

    //printf("Please enter the message: ");
    memset(buffer,'\0',BUFSIZE);
    strcpy(buffer, message);
    //n = write(sockfd,buffer,strlen(buffer));
    sent = send(sockfd, buffer, BUFSIZE-1, 0);
    if (sent < 0) {
      fprintf(stderr, "ERROR writing to socket\n");
      exit(1);
    }
    memset(buffer,'\0',BUFSIZE);
    //n = read(sockfd,buffer,BUFSIZE-1);
    received = recv(sockfd, buffer, BUFSIZE-1, 0);
    if (received < 0) {
      fprintf(stderr, "ERROR reading from socket\n");
      exit(1);
    }
    printf("%s",buffer); // DO NOT ADD "\n", make sure client printout the exact message from the server, solved #1 failed test
    return 0;
}
