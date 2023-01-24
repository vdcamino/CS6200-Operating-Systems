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
    "  transferclient [options]\n"                           \
    "options:\n"                                             \
    "  -s                  Server (Default: localhost)\n"    \
    "  -p                  Port (Default: 50419)\n"           \
    "  -o                  Output file (Default cs6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 50419;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:hx", gLongOptions, NULL)) != -1)
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
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
    int sockfd, recv_file, data_recv, data_to_write;
    struct sockaddr_in serv_addr;
    struct hostent *server;


    char buffer[BUFSIZE];
    /* Open the socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0); /* IP address specifically*/
    if (sockfd < 0){
      fprintf(stderr, "ERROR opening socket\n");
      exit(1);
    }
    /* Obtain the hostname of the server*/
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }
    /* Start to connect to the server*/
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      fprintf(stderr, "ERROR connecting\n");
      exit(1);
    }
    /*Create or open the received file */
    recv_file = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (recv_file < 0) {
      fprintf(stderr, "ERROR opening a file\n");
      exit(1);
    }
    /* Start to receive the data from the server */
    do{
      memset(buffer,'\0',BUFSIZE);
      data_recv = recv(sockfd, buffer, BUFSIZE, 0);
      if (data_recv < 0) {
        fprintf(stderr, "ERROR receiving data from socket\n");
        exit(1);
      }
      data_to_write = data_recv;
      if(write(recv_file, buffer, data_recv)!=data_to_write){
        fprintf(stderr, "ERROR writing data to the file\n");
        exit(1);
      }
    } while (data_recv!=0);
    printf("File received successfully!\n");
    close(recv_file);
    close(sockfd);

    return 0;
}
