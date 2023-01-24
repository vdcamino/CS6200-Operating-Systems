#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>

#define BUFSIZE 5041

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: 6200.txt)\n" \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 50419)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};
void transfer_file(int, char*); /* function prototype */
int main(int argc, char **argv)
{
    int option_char;
    int portno = 50419;             /* port to listen on */
    char *filename = "6200.txt"; /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:hf:x", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'f': // listen-port
            filename = optarg;
            break;
        }
    }


    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    int sockfd, newsockfd, clilen, pid;
    struct sockaddr_in serv_addr, cli_addr;
    int serv_addrSize=sizeof(struct sockaddr_in);
    // create a new socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
      fprintf(stderr, "ERROR opening socket");
      exit(1);
    }
    // enable SO_REUSEADDR option
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
      // Establish a connection with a client
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
        transfer_file(newsockfd,filename);
        exit(0);
      }
      else close(newsockfd);
    }
    return 0;
}

/******** transfer_file() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void transfer_file (int sock, char* filename)
{
   char buffer[BUFSIZE];
   int sent_file, data_read, data_to_send, data_sent;
   // open the file
   sent_file = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
   if (sent_file < 0) {
     fprintf(stderr, "ERROR opening a file\n");
     exit(1);
   }
        /* Start to send the data from the server */
   do{
     memset(buffer, '\0', BUFSIZE);
     //read data into buffer, cannot gurantee read BUFSIZE data into buffer
     data_read = read(sent_file, buffer, BUFSIZE-1);
     if (data_read < 0) {
       fprintf(stderr, "ERROR reading a file\n");
       exit(1);
     }
     // send data chunk by chunk
     char *p = buffer; // set a pointer to record the location of sent data
     data_to_send = data_read;
     while (data_to_send > 0){
       data_sent = send(sock, p, data_to_send, 0);
       if (data_sent < 0) {
         fprintf(stderr, "ERROR writing to socket\n");
         exit(1);
       }
       data_to_send -= data_sent;
       p += data_sent;
     }
   } while(data_read!=0);
   printf("Transfer file successfully!\n");
}
