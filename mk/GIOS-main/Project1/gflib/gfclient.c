
#include "gfclient-student.h"
struct  gfcrequest_t
{
   char * scheme;
    const char *server;
    const char *path;
    unsigned short portno;
    void *writearg;
    char *status_server;
     gfstatus_t status;
    void (*writefunc)(void*, size_t, void *);
    void *headerarg;
    void (*headerfunc)(void*, size_t, void *);
    size_t bytes_received;
    size_t filelength;
    int count;
};

gfcrequest_t *gfc_create(){
     gfcrequest_t *request;
     //// mallocing due to the need for persistent client request.
    request=(gfcrequest_t*) malloc(sizeof(gfcrequest_t)); 
   if(request !=NULL) return request;
    else return (gfcrequest_t*)NULL;
    
}
// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr){
    free(*gfr);
    (*gfr)=NULL;
}



size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
   
    return (*gfr)->bytes_received;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
    
    return (*gfr)->filelength;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){
    gfstatus_t status;
    if((*gfr)->status ==0){status = GF_OK;}
    else if ((*gfr)->status ==1){status = GF_FILE_NOT_FOUND;}
    else if ((*gfr)->status ==2){status = GF_ERROR;}
    else {status = GF_INVALID;}
   return status;
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t **gfr){
    int client_socket,result;
    struct addrinfo hints,*addrs,*temp;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_family=AF_UNSPEC;
    char port[10];
    snprintf (port, sizeof(port), "%d",(*gfr)->portno);
    result=getaddrinfo((*gfr)->server,port,&hints,&addrs);
    if(result==-1){perror("Error in getaddrinfos----\n");}
    int bytes_sent=0;

    for(temp=addrs;temp!=NULL;temp=temp->ai_next)
    {
        // setting the socket
        client_socket= socket(temp->ai_family,temp->ai_socktype,0);
        if(client_socket ==-1)
        {
            perror("Error creating socket!!!!");
            continue;
        }
         if(connect(client_socket,temp->ai_addr,temp->ai_addrlen)==-1)
            {
                perror("Connect error:");
                continue;
            }
        printf("Connection Successful....\n");
        break;
    }
    freeaddrinfo(addrs);
    // creating the request string 
    char request [255];
    
    memset(&request,'\0',sizeof(request));
    strcat(request,SCHEME);
    strcat(request,METHOD);
   strcat(request,(*gfr)->path);
    strcat(request,MARKER);
    
    printf("SENDING REQUEST....\n");
    
    while (bytes_sent < strlen(request))
    {
       
        bytes_sent= send(client_socket,request,strlen(request),0);
        if(bytes_sent == -1){perror("Error sending request"); break;}
        if(bytes_sent == 0){break;}
    }
        printf("Bytes sent ---->%d\n",bytes_sent);
    /**********************FETCHING DATA FROM THE SERVER**************************************************/
     void *ptr=malloc(sizeof(char) *BUFSIZE);
        char *cptr=(char*)ptr;
     char response[255];
     char buffer[255];
     memset(&buffer,'\0',255);
     memset(&response,'\0',255);
    memset(ptr,'\0',BUFSIZE);
     size_t response_bytes=0;
     size_t total=0;
     int firstResponse=0;
    while (1)
    {
           // printf("GETTING THE SERVER RESPONSE ------------------------------>\n");
            response_bytes = recv(client_socket,cptr,255,0);
            if(response_bytes == 0) {printf("Nothing received"); 
                (*gfr)->bytes_received=response_bytes;
                free(ptr);
                return -1;
               
            }
            if(response_bytes == -1){ perror("Error receiving response from the server");return -1;}
            memcpy(response,cptr,sizeof(response));
            //  EXTRACTING THE HEADER ....
            if(firstResponse == 0)
            {
                
                    for (int j=0;j<response_bytes;j++)
                    {
                        if(response[j]!='\r' && response[j+1]!='\n') {buffer[j]=response[j];}
                        else{break; }
                    }
                    
                   // printf("RESPONSE HEADER RECEIVED--->%ld\n",strlen(buffer)+4);
                   // CALCULATING THE CONTENT SIZE
                        response_bytes = response_bytes -(strlen(buffer)+4);

                    // SPLITTING THE HEADER
                        char * scheme =strtok(buffer," ");
                       // printf("The scheme---->%s\n",scheme);
                        (*gfr)->scheme =scheme;
                        char * status= strtok(NULL," ");
                       // printf("The status---->%s\n",status);
                //IF STATUS OK
                if(strcmp((*gfr)->scheme,"GETFILE")!=0){(*gfr)->status=GF_INVALID; free (ptr);return -1;}
                if(strcmp(status,"OK")==0)
                    {
                         cptr=strstr((char*)ptr,"\r\n\r\n");
                        cptr=cptr+4;
                        char * filelength= strtok(NULL," ");
                        (*gfr)->status=GF_OK;
                        (*gfr)->filelength=atol(filelength);
                        (*gfr)->writefunc(cptr,response_bytes,(*gfr)->writearg);
                        total +=response_bytes;
                        firstResponse=1;
                    }
                
                //IF STATUS FILE_NOT_FOUND

                if(strcmp(status,"FILE_NOT_FOUND")==0){(*gfr)->status=GF_FILE_NOT_FOUND; 
                (*gfr)->bytes_received=response_bytes;firstResponse=1;
                free(ptr);return 0;}

                //IF STATUS ERROR
                
                if(strcmp(status,"ERROR")==0){(*gfr)->status=GF_ERROR;
                (*gfr)->bytes_received=response_bytes;firstResponse=1;
                free(ptr); return 0;}

                //IF STATUS INVALID 
                if(strcmp(status,"INVALID")==0){(*gfr)->status=GF_INVALID;
                (*gfr)->bytes_received=response_bytes;firstResponse=1;
                 free(ptr); return -1;}

                 

            }
            // WRITING THE REMAINING FILE CONTENT
            else
            { 
                (*gfr)->writefunc(cptr,response_bytes,(*gfr)->writearg);
                total +=response_bytes;
                firstResponse=1;
            }
            
            
            //printf("TOTAL BYTES------->%ld\n",total);
            if(total == (*gfr)->filelength)
            {
                //printf("REACHED THE FILELENGTH------>\n");
                (*gfr)->bytes_received=total;    
                break;
            }
        
    }
   
    free(ptr);
    close(client_socket);
    return 0;
}



void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg){
    (*gfr)->headerarg=headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *)){
        (*gfr)->headerfunc=headerfunc;
}

void gfc_set_path(gfcrequest_t **gfr, const char* path){
   (*gfr)->path=path;
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port){
    (*gfr)->portno=port;
}

void gfc_set_server(gfcrequest_t **gfr, const char* server)
{
    (*gfr)->server=server;
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg){
    (*gfr)->writearg=writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
  (*gfr)->writefunc=writefunc;
}

const char* gfc_strstatus(gfstatus_t status){
    const char *strstatus = NULL;

    switch (status)
    {
        default: {
            strstatus = "UNKNOWN";
        }
        break;

        case GF_INVALID: {
            strstatus = "INVALID";
        }
        break;

        case GF_FILE_NOT_FOUND: {
            strstatus = "FILE_NOT_FOUND";
        }
        break;

        case GF_ERROR: {
            strstatus = "ERROR";
        }
        break;

        case GF_OK: {
            strstatus = "OK";
        }
        break;
        
    }

    return strstatus;
}

