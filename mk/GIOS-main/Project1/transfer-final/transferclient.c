#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/fcntl.h>

#include <netinet/in.h>
#include <sys/types.h>

#define BUFSIZE 820

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferclient [options]\n"                           \
    "options:\n"                                             \
    "  -s                  Server (Default: localhost)\n"    \
    "  -p                  Port (Default: 20801)\n"           \
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
    unsigned short portno = 20801;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:xp:o:h", gLongOptions, NULL)) != -1)
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
     int client_socket,result;
   
    struct addrinfo specs,*addrs,*temp;

    // clearing all the previous values if any
    memset(&specs,0,sizeof(specs));
    specs.ai_family= AF_UNSPEC; // ipv4 or ipv6
    specs.ai_socktype=SOCK_STREAM;
    char port[10];
    snprintf (port, sizeof(port), "%d",portno);
   
   // printf("Port----->%s\n",port);
    result=getaddrinfo(hostname,port,&specs,&addrs);

    if(result==-1){
        perror("Getaddrinfo error:");
    }

    for(temp=addrs;temp!=NULL;temp=temp->ai_next)
    {
        client_socket=socket(temp->ai_family,temp->ai_socktype,0);
        if(client_socket==-1)
            {
                perror("Socket set failed");
               continue;
            }
            //printf("Socket set-----------------\n");
            
            if(connect(client_socket,temp->ai_addr,temp->ai_addrlen)==-1){
               perror("Connection failed");
                continue;
             }
              //printf("Connecting to server------------------\n");
             break;
    }
     if(temp==NULL){
       //  printf("Null addrinfo");
     }
    freeaddrinfo(addrs);
    /*************************************/
                int size=BUFSIZE; 
            void * ptr =malloc(sizeof(char)* BUFSIZE);
            int bytes_received=0;
            int bytes_written=0;
            int bytes_remaining= size;
            int *recv_ptr= (int *)ptr;   
            memset(recv_ptr,'\0',BUFSIZE);
    /************************************/
    int write_fd=open(filename,O_WRONLY|O_CREAT,S_IWUSR);
    if(write_fd ==-1){ perror("Error opening file !!!!!\n");}
       // printf("Before while 1");
        for(;;)
        {
           //printf("Preparing to receive --- --->\n");
            bytes_received= recv(client_socket,recv_ptr,bytes_remaining,0);
           //printf("Received--- ---> %d\n",bytes_received);
            if ( bytes_received == 0){//printf(" Nothing to receive-------->");
             break;}
            if ( bytes_received == -1){perror(" Error receiving data !!"); break;}
            
            while (bytes_received > bytes_written)
            {
                bytes_written = write(write_fd ,recv_ptr,bytes_received-bytes_written);
               //printf("Bytes written --------->%d\n",bytes_written);
                if (bytes_written == 0){//printf(" Nothing to write ------->"); 
                break;}
                if (bytes_written == -1){perror ("Error writing to file !!!"); break;}
                bytes_received-=bytes_written;
                recv_ptr = ptr+bytes_received;
            }
            bytes_written=0;
            memset(ptr,'\0',BUFSIZE);

        }
          free(ptr);
    close(client_socket);   
    }
