#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/fcntl.h>

#define BUFSIZE 820

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: 6200.txt)\n" \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 20801)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv)
{
    int option_char;
    int portno = 20801;             /* port to listen on */
    char *filename = "6200.txt"; /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "xp:hf:", gLongOptions, NULL)) != -1)
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
        case 'f': // file to transfer
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

    int sock_fd,cli_fd,status;
    socklen_t cddr_len;
    struct addrinfo hostaddr,*result,*temp;
    memset(&hostaddr,0,sizeof(hostaddr));

    struct sockaddr_storage client_addr;
    int option=1;
    hostaddr.ai_family=AF_UNSPEC;
    hostaddr.ai_socktype=SOCK_STREAM;
    hostaddr.ai_flags=AI_PASSIVE;
    char port[10];
    snprintf(port,sizeof(port),"%d",portno);
    status= getaddrinfo(NULL,port,&hostaddr,&result);

    if(status==-1){perror("Problem Getaddrinfo");}
    
    for(temp=result;temp!=NULL;temp=temp->ai_next)
    {
        if((sock_fd=socket(temp->ai_family,temp->ai_socktype,temp->ai_protocol))==-1){
            perror("Socket creation:");
            continue;
        }
       //printf("Socket created-------------\n");
        if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(int))==-1){
            perror("Socket resuse");
            exit(1);
        }
        if(bind(sock_fd,temp->ai_addr,temp->ai_addrlen)==-1){
             perror("Binding:");
             continue;
        }
        //printf("Binding successful------------\n");
        break;
    }
    
    freeaddrinfo(result);
   
    if(listen(sock_fd,3)==-1)
    {
        perror("Listen error");
    }
 // printf("WAiting for connections--------\n");
    
    /*********************/
            int bytes_remaining=100;         
            void *ptr=malloc(sizeof(char)* BUFSIZE); // handle error if malloc not successful.
            int bytes_sent=0;
            int bytes_read=0;
           int *cur_ptr=(int*)ptr;
           memset(cur_ptr,'\0',BUFSIZE);
           int n;
    /*********************/
    while(1)
    {
            cddr_len=sizeof(client_addr);
            cli_fd=accept(sock_fd,(struct sockaddr*) &client_addr,&cddr_len);
             // set to avoid accept blocking the socket until all info is sent/received
            //fcntl(cli_fd,F_SETFL,O_NONBLOCK);
            if(cli_fd==-1)
            {
                perror("Accepting connections error");
                continue;
            }

           int fd=open(filename,O_RDONLY,S_IRUSR);
           if(fd==-1)
            {            
                perror("Error opening file");
            }
            /***********************
             * CALLING THE READ  AND SEND OPERATION
             * ****************************/
            for(;;)
            {
               
                bytes_read= read(fd,cur_ptr,bytes_remaining);
                //printf("Bytes Read-------------------->%d\n",bytes_read);
                if(bytes_read == -1){ perror (" Reading file error ----->\n");}
                if( bytes_read == 0){ //printf(" Nothing to read ------>\n") ;
                close(cli_fd);break; }
                while ( bytes_read > bytes_sent)
                {
                   // printf("Preparing to send");
                    n = send(cli_fd , cur_ptr,bytes_read - bytes_sent,0);
                    // check for n
                    if(n == 0){printf(" Nothing else to send------------->\n");
                    close(cli_fd);break;}
                    if(n == -1){perror("Error sending contents------------>");break;}
                    bytes_sent +=n;
                    bytes_read -=n;
                   cur_ptr =ptr +bytes_sent;
                   // printf(" Bytes sent ----------------->%d\n",bytes_sent);
                    
                }
                bytes_sent=0;
                
            }
       free(ptr);
       close(cli_fd);  
    }
    
    
}
 