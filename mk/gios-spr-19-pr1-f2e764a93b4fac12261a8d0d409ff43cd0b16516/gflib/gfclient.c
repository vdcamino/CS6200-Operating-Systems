#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "gfclient-student.h"
#define PATH_BUFFER_SIZE 504
#define BUFSIZE 5041
#define HEADER_BUFFER_SIZE 50
// #define TIMER 5

/* Define the request struct*/
struct gfcrequest_t{
  size_t bytesreceived; /* actual number of received bytes */
  size_t filelen; /* file length from server's response */
  gfstatus_t status; /* status, enum t(ype */
  const char *server; /* server */
  const char *path; /* requested path */
  char *response; /* response from the server */
  char *content; /* content from the server */
  unsigned short portno; /* port number */
  void (*headerfunc)(void*, size_t, void *); /* header function */
  void *headerarg; /* header arguments */
  void (*writefunc)(void*, size_t, void *); /* write function */
  void *writearg; /* write arguments */
};

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t *gfr){
  free(gfr);
}

gfcrequest_t *gfc_create(){
    // dummy for now - need to fill this part in
    /* allocate the size for client's request */
    struct gfcrequest_t *gfr = (struct gfcrequest_t*) malloc(sizeof(struct gfcrequest_t));
    /* Initialization*/
    gfr->bytesreceived=0; /* actual number of received bytes */
    gfr->filelen=0; /* file length from server's response */
    gfr->status=GF_OK; /* status, enum t(ype */
    gfr->server=NULL; /* server */
    gfr->path=NULL; /* requested path */
    gfr->response=NULL; /* response from the server */
    gfr->content=NULL; /* content from the server */
    gfr->portno=0; /* port number */
    gfr->headerfunc=NULL; /* header function */
    gfr->headerarg=NULL; /* header arguments */
    gfr->writefunc=NULL; /* write function */
    gfr->writearg=NULL; /* write arguments */
    return gfr;

}

size_t gfc_get_bytesreceived(gfcrequest_t *gfr){
    // not yet implemented
      return gfr->bytesreceived;
}

size_t gfc_get_filelen(gfcrequest_t *gfr){
    // not yet implemented
      return gfr->filelen;
}

gfstatus_t gfc_get_status(gfcrequest_t *gfr){
    // not yet implemented
    return gfr->status;
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t *gfr){
    // currently not implemented.  You fill this part in.
    int sockfd, rv;
    // struct sockaddr_in serv_addr;
    // struct hostent *server;
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, '\0' , sizeof(hints));
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
      fprintf(stderr, "ERROR opening socket\n");
      exit(1);
    }
    char portstr[10];
    sprintf(portstr, "%u", gfr->portno);
    /* change from gethostbyname to getaddrinfo, for IPv6 support*/
    if((rv = getaddrinfo(gfr->server, portstr, &hints, &servinfo))!=0){
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      exit(1);
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
          perror("socket");
          continue;
        }

      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          perror("connect");
          close(sockfd);
          continue;
        }
        break; // if we get here, we must have connected successfully
    }

    if (p == NULL) {
        // looped off the end of the list with no connection
        fprintf(stderr, "failed to connect\n");
        exit(1);
    }
    freeaddrinfo(servinfo);
    // server = gethostbyname(gfr->server); // for ipv4 use only

    // if (server == NULL) {
    //     fprintf(stderr,"ERROR, no such host\n");
    //     exit(1);
    // }
    // bzero((char *) &serv_addr, sizeof(serv_addr));
    // serv_addr.sin_family = AF_INET;
    // bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    // serv_addr.sin_port = htons(gfr->portno);

    // if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
    //   fprintf(stderr, "ERROR connecting\n");
    //   gfr->status = GF_ERROR;
    //   exit(1);
    // }

    /* construct a request mesage from the client*/
    char *scheme = "GETFILE ";
    char *method = "GET ";
    char *endmarker = "\r\n\r\n";

    char buffer[BUFSIZE];
    memset(buffer, '\0', BUFSIZE);
    strcpy(buffer, scheme);
    strcat(buffer, method);
    strcat(buffer, gfr->path);
    strcat(buffer, endmarker);
    //printf ("Send a request from client: \s\n", buffer);

    ssize_t sent = send(sockfd, buffer, strlen(buffer), 0);
    if (sent < 0) {
      fprintf(stderr, "ERROR writing to socket\n");
      exit(1);
    }

    /*
     * receive header and file contents
     * first, check the header validity
     * second, check the status and length
     * third, receive the content, record the length
     */
    size_t total_recv = 0;
    ssize_t data_recv = 0;
    size_t file_recv = 0;
    int is_header_received = 0;
    char header[HEADER_BUFFER_SIZE];
    char recv_buffer[BUFSIZE];
    char content_buffer[BUFSIZE];
    size_t file_len;
    int flag = 0;
    do{
      memset(recv_buffer,'\0',BUFSIZE);
      memset(content_buffer,'\0',BUFSIZE);
      data_recv = recv(sockfd, recv_buffer, BUFSIZE-1, 0);
      if (data_recv < 0) {
        fprintf(stderr, "ERROR receiving data from socket\n");
        exit(1);
      }
      /* use content_buffer to locate the start of the contents*/
      memcpy(content_buffer,recv_buffer,data_recv);
      total_recv += data_recv;
      gfr->response = recv_buffer;

      // printf("Start Here--------------\n");
      /* if header is not received, go to deal with header */
      if(is_header_received == 0){
        /* server closed socket in header transfer*/
        if (data_recv == 0) {
          fprintf(stderr, "ERROR receiving data from socket\n");
          gfr->status = GF_ERROR;
          break;
        }
        /* malformed header */
        if(strstr(recv_buffer,scheme)==NULL||strstr(recv_buffer,endmarker)==NULL){
          gfr->status = GF_INVALID;
          // return -1;
          break;
        }
        // printf("======recv_buffer is:%s. How many:%zu=========\n",recv_buffer, data_recv);
        char *buffer_header = strtok(recv_buffer, "\r\n\r\n");
        if(buffer_header==NULL) {
          gfr->status = GF_INVALID;
          break;
        }

        // printf("====buffer header is %s====\n", buffer_header);
        char recv_status[20];
        memset(recv_status,'\0',20);

        // flag=2 when status and file_len exist; flag=1 when only status exists
        flag = sscanf(buffer_header, "GETFILE %s %zu\r\n\r\n", recv_status,&file_len); // has status and file length
        if(flag!=2){
          flag = sscanf(buffer_header, "GETFILE %s\r\n\r\n", recv_status); // only has status
        }

        if (flag==1) {
          is_header_received = 1; //received header with status FNF, ERROR, or INVALID
          if(strcmp(recv_status, "FILE_NOT_FOUND")==0){
            gfr->status = GF_FILE_NOT_FOUND;
          } else if (strcmp(recv_status, "INVALID")==0){
            gfr->status = GF_INVALID;
          } else{
            gfr->status = GF_ERROR;
          }
          break;
        } else if (flag == 2){
          is_header_received = 1;
          if(strcmp(recv_status, "OK")==0){
            // printf("====3rd Status is %s===\n", recv_status);
            gfr->status = GF_OK; //received header with status OK and file length
            gfr->filelen = file_len;
          }else{
            gfr->status = GF_INVALID;
            break;
          }
        } else {
          gfr->status = GF_INVALID; //received header with others, regarding as invalid
          break;
        }


        memset(header, '\0', HEADER_BUFFER_SIZE);
        sprintf(header, "GETFILE %s %zu\r\n\r\n", recv_status, gfr->filelen);
        // printf("=====I generate a header as is %s====\n", header);

        file_recv = file_recv+data_recv-strlen(header);
        gfr->bytesreceived = file_recv;
        /* write content after the header*/
        if(gfr->writefunc!=NULL&&gfr->status == GF_OK) {
          gfr->writefunc(content_buffer+strlen(header), data_recv - strlen(header), gfr->writearg);
        }
      } else {
        // printf("====== How many:%zu=======\n", data_recv);
        if (data_recv < 0) {
          fprintf(stderr, "ERROR receiving data from socket\n");
          exit(1);
        }
        /* server closed socket in content transfer*/
        if (data_recv ==0 && file_recv<file_len){
          gfr->status = GF_OK;
          // return -1;
          break;
        }
        /* write content */
        gfr->writefunc(content_buffer, data_recv, gfr->writearg);
        gfr->bytesreceived = file_recv;
        file_recv = file_recv + data_recv;
      }

    }while(file_len>file_recv); // keep receiving data until file_len==file_recv


    gfr->bytesreceived = file_recv;
    /* based on status and file_recv, decide the return return_code
       ERROR, FNF, OK with correct file_recv: return 0
       INVALID, OK with incorrect file_recv, and others: return -1
    */
    int return_code;
    switch (gfr->status)
    {
      case GF_INVALID:{
        return_code = -1;
      }
      break;
      case GF_FILE_NOT_FOUND:{
        return_code = 0;
      }
      break;
      case GF_ERROR:{
        return_code = 0;
      }
      break;
      case GF_OK:{
        if(data_recv ==0&&file_recv!=gfr->filelen){
          return_code = -1; // if the socket is closed during data transfer
        } else if(file_recv==gfr->filelen){
          return_code = 0;
        }
      }
      break;
      default:
        return_code = -1;
      break;
    }
    return return_code;
  }

void gfc_set_headerarg(gfcrequest_t *gfr, void *headerarg){
  gfr->headerarg = headerarg;

}

void gfc_set_headerfunc(gfcrequest_t *gfr, void (*headerfunc)(void*, size_t, void *)){
  gfr->headerfunc = headerfunc;
}

void gfc_set_path(gfcrequest_t *gfr, const char* path){
  gfr->path = path;
}

void gfc_set_port(gfcrequest_t *gfr, unsigned short port){
  gfr->portno = port;
}

void gfc_set_server(gfcrequest_t *gfr, const char* server){
  gfr->server = server;
}

void gfc_set_writearg(gfcrequest_t *gfr, void *writearg){
  gfr->writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t *gfr, void (*writefunc)(void*, size_t, void *)){
  gfr->writefunc = writefunc;
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
