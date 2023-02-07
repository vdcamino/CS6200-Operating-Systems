
#include "gfserver-student.h"

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */



// defining the gfserver_t struct
struct gfserver_t
{
    gfh_error_t (*handlerfunc)(gfcontext_t **, const char *, void*);
    unsigned short portno;
    int max_npending;
    void* arg;
    char * filepath;
    char *scheme;
    char *method;
};

// defining the struct gfcontext_t
 struct gfcontext_t
 {
     int client_socket;
     char* filepath;
    int status;
    size_t file_len;
    size_t total;
    int count;
 };
void gfs_abort(gfcontext_t **ctx){
   printf("status from context----->%d",(*ctx)->status);
    close((*ctx)->client_socket);
}
gfserver_t* gfserver_create(){
     
   gfserver_t* server= (gfserver_t*)malloc (sizeof(gfserver_t));
    if(server !=NULL)
    { return server;}
    return (gfserver_t *)NULL;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){

    size_t bytes_sent=0;
    while(bytes_sent < len){
        bytes_sent = send((*ctx)->client_socket,data,len,0);
        if(bytes_sent ==-1){perror("Error sending the response"); }
         if(bytes_sent == 0){printf("Nothing to send");break;}
         len-=bytes_sent;
    }
    
return bytes_sent;
}
ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len)
{
    
    ssize_t bytes_sent=0;
   printf(" Inside send header ------------->%d\n",status);
   char *response;
     if (status == GF_FILE_NOT_FOUND)
    {   
        response= "GETFILE FILE_NOT_FOUND \r\n\r\n";
        bytes_sent = send((*ctx)->client_socket,response,strlen(response),0);
        printf("Error bytes sent----%ld\n",bytes_sent);
        
    }
    else if (status == GF_OK)
    {
        char filelength[500]; // to make it ascii number
        sprintf(filelength, "%zu", file_len);  
        (*ctx)->file_len = file_len;
       char response[255];
        strcpy(response, "GETFILE OK ");
        strcat(response, filelength);
        strcat(response, " \r\n\r\n");
        printf("Sending file length %s\n",filelength);
        bytes_sent = send((*ctx)->client_socket, response, strlen(response),0);
       
        
    }
   else if (status==GF_ERROR)
   {
       response= "GETFILE ERROR \r\n\r\n";
        bytes_sent = send((*ctx)->client_socket,response,strlen(response),0);
        printf("Error bytes sent----%ld\n",bytes_sent);
        
   }
    return bytes_sent;
}

void gfserver_serve(gfserver_t **gfs){
    
    int result;
    int server_socket,client_socket;
   struct addrinfo hints,*addrs,*temp;
  
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_family=AF_UNSPEC;
    hints.ai_flags=AI_PASSIVE;
    char port[10];
    snprintf (port, sizeof(port), "%d",(*gfs)->portno);
    int option =1;
    result=getaddrinfo(NULL,port,&hints,&addrs);
    if(result==-1){perror("Error in getaddrinfos----\n");}
    for(temp=addrs;temp!=NULL;temp=temp->ai_next)
    {
        if((server_socket = socket(temp->ai_family,temp->ai_socktype,0))==-1)
        {
            perror("Error creating socket");
            continue;
        }
        if(setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&option,sizeof(int))==-1){
            perror("Socket resuse");
            exit(1);
        }
         // binding to the server socket
        if(bind(server_socket,temp->ai_addr,temp->ai_addrlen)==-1)
        {
            perror("Binding error:");
            continue;
        }
         printf("Binding Successful --->\n");
    }
   freeaddrinfo(addrs);
    //listening
    if(listen(server_socket,(*gfs)->max_npending)==-1)
    {
        perror("Error listening !!!");
    }
    printf(" Listening successful------>\n");
    
    
    ///////////SETTING THE SOCKET DESP FOR THE CONTEXT FOR READ AND SEND FILE INFO//////////////////
    
     struct sockaddr_storage client_address;   
     socklen_t addr_size;
      size_t bytes_received =0;
    void *ptr=malloc(sizeof(char) *BUFSIZE);
        char *cptr=(char*)ptr;
        char request[255];
     char buffer[255];
     // while for continuosly accept further requests.
    while(1) {
                     gfcontext_t *ctx = malloc(sizeof(gfcontext_t));
                    printf("Preparing to process request--> \n");
                    memset(&buffer,'\0',255);
                    memset(&request,'\0',255);
                    memset(ptr,'\0',BUFSIZE);
                    addr_size =sizeof(client_address);
                    client_socket=accept(server_socket,(struct sockaddr*)&client_address,&addr_size);
                        if(client_socket==-1)
                        {
                        perror("Accept failed:");
                        continue;
                        }
                
                // while for receiving one request at a time.
                 while(1){
                     
                    bytes_received = recv(client_socket,cptr,255,0);
                    if(bytes_received == 0) {printf("Nothing received\n"); break;}   
                    if(bytes_received == -1){ perror("Error receiving request from the client\n");break;}
                     memcpy(request,cptr,sizeof(request));
                    
                     // checking byte-by-byte to make sure received the request marker.
                         for (int j=0;j<bytes_received;j++)
                            {
                                if(request[j]!='\r' && request[j+1]!='\n') {buffer[j]=request[j];}
                                else{break; }
                            }
                            char * scheme =strtok(buffer," ");
                                printf("The scheme---->%s\n",scheme);
                                (*gfs)->scheme =scheme;
                                char * method= strtok(NULL," ");
                                (*gfs)->method=method;
                                
                                char *filepath=strtok(NULL,"\r\n\r\n");
                                printf("method RECEIVED------>%s\n",method);
                                printf("filepath RECEIVED------>%s\n",filepath);
                                printf("scheme RECEIVED------>%s\n",scheme);
                                (*gfs)->filepath=filepath;
                                
                                if((strcmp((*gfs)->scheme,"GETFILE")!=0) || (strcmp((*gfs)->method,"GET") !=0))
                                {
                                    // INVALID HEADER
                                   
                                    char * response ="GETFILE INVALID\r\n\r\n";
                                    send(client_socket,response,strlen(response),0);
                                    
                                     break;
                                }
                                if ((*gfs)->filepath!=NULL)
                                {
                                    if(strncmp((*gfs)->filepath,"/",1)!=0){
                                         char * response ="GETFILE INVALID\r\n\r\n";
                                    send(client_socket,response,strlen(response),0);
                                    
                                     break;
                                    }
                                }
                                
                                    (*ctx).client_socket=client_socket;
                                     (*gfs)->handlerfunc(&ctx,(*gfs)->filepath,(*gfs)->arg);
                                   break;
                                                                                           
                        } // finished processing request.
              free(ctx); // freeing the context once a request is handled.
         }
    free(ptr); // freeing the buffer used for storing the request info.
            free(*gfs);
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
        (*gfs)->arg=arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
        (*gfs)->handlerfunc=handler;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
        (*gfs)->max_npending=max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*gfs)->portno=port;
}


