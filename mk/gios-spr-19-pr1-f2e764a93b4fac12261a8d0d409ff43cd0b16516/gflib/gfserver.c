#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "gfserver-student.h"
#include "gfserver.h"
/*
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */
#define MAX_REQUEST_LEN 128
#define BUFSIZE 5041
#define SCHEME_BUFSIZE 20
#define METHOD_BUFSIZE 30
#define PATH_BUFSIZE 300
#define TIMER 5
typedef int gfstatus_t;

#define  GF_OK 200
#define  GF_FILE_NOT_FOUND 400
#define  GF_ERROR 500
#define  GF_INVALID 600

 struct gfserver_t {
     unsigned short portno;
     int maxnpending;
     ssize_t (*handler)(gfcontext_t *,const char *, void*);
     void *handlerarg;
 };

 struct gfcontext_t {
     int sockfd; /* listening socket */
     int newsockfd; /* connection socket */
     char *path; /* requested path*/
     // char *request; /* request message from client*/
 };

void gfs_abort(gfcontext_t *ctx){
  fprintf(stdout, "Abort the connection.\n");
  close(ctx->newsockfd);
}

/*
 * This function must be the first one called as part of
 * setting up a server.  It returns a gfserver_t handle which should be
 * passed into all subsequent library calls of the form gfserver_*.  It
 * is not needed for the gfs_* call which are intended to be called from
 * the handler callback.
 */
gfserver_t *gfserver_create(){
    /* allocate the size for server's struct */
  struct gfserver_t *gfs = (struct gfserver_t*) malloc(sizeof(gfserver_t));
  gfs->portno = 0;
  gfs->maxnpending = 1;
  gfs->handler = NULL;
  gfs->handlerarg = NULL;
  return gfs;
}


ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){
  /* send header; len_scheme + len_status + file_len */
  ssize_t sent_header;
  char sheader[MAX_REQUEST_LEN];
  memset(sheader, '\0', MAX_REQUEST_LEN);
  switch (status) {
    case GF_OK:
      sprintf(sheader,"GETFILE OK %zu\r\n\r\n", file_len);
      printf("GF is OK, file length is %zu\n", file_len);
      sent_header = send(ctx->newsockfd, sheader, strlen(sheader), 0);
      break;

    case GF_FILE_NOT_FOUND:
      strcpy(sheader,"GETFILE FILE_NOT_FOUND\r\n\r\n");
      printf("GF is FILE_NOT_FOUND\n");
      sent_header = send(ctx->newsockfd, sheader, strlen(sheader), 0);
      break;

    case GF_ERROR:
      strcpy(sheader,"GETFILE ERROR\r\n\r\n");
      printf("GF is ERROR\n");
      sent_header = send(ctx->newsockfd, sheader, strlen(sheader), 0);gfs_abort(ctx);
      break;

    default:
      strcpy(sheader,"GETFILE INVALID\r\n\r\n");
      printf("GF is INVALID\n");
      sent_header = send(ctx->newsockfd, sheader, strlen(sheader), 0);
      break;
  }
  return sent_header;
}

// send data
ssize_t gfs_send(gfcontext_t *ctx, const void *data, size_t len){
    ssize_t data_sent = send(ctx->newsockfd, data, len, 0);
    return data_sent;
}


void gfserver_serve(gfserver_t *gfs){
  int sockfd, newsockfd, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  char scheme[SCHEME_BUFSIZE];
  char method[METHOD_BUFSIZE];
  char path[PATH_BUFSIZE];
  char buffer[BUFSIZE];
  char temp[BUFSIZE];
  int serv_addrSize=sizeof(struct sockaddr_in);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0){
    fprintf(stderr, "ERROR opening socket.\n");
    exit(1);
  }
  // enable SO_REUSEADDR option
  int enable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
    fprintf(stderr,"setsockopt(SO_REUSEADDR) failed.\n");
    exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(gfs->portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    fprintf(stderr, "ERROR on binding.\n");
    exit(1);
  }
  getsockname(sockfd, (struct sockaddr *) &serv_addr,(socklen_t *)&serv_addrSize);
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  while(1){
    /* establish a connection to the client*/
    newsockfd = accept(sockfd, (struct sockaddr *) & cli_addr, (socklen_t *)&clilen);
    if (newsockfd < 0){
      fprintf(stderr, "ERROR on accept\n");
      exit(1);
    }
    gfcontext_t *gfc = (gfcontext_t *)malloc(sizeof(gfcontext_t));
    gfc->sockfd = sockfd;
    gfc->newsockfd = newsockfd;

    memset(buffer,'\0',BUFSIZE);
    // strcpy(buffer,"");
    int timer = 0;
    do{
      memset(temp,'\0',BUFSIZE);
      /* Receive the request from the client*/
      ssize_t received = recv(newsockfd, temp, BUFSIZE-1, 0);
      printf("I received %zu data from clients---\n", received);
      if (received <0){
        fprintf(stderr, "ERROR reading from socket\n");
        exit(1);
      }
      if (timer==0) {
        memcpy(buffer, temp, received);
      } else {
        strcat(buffer,temp);
      }
      timer = timer + 1; // keep receiving until hit the end marker or timer expires
    }while(strstr(buffer,"\r\n\r\n")==NULL&&timer<TIMER); // in case the header is super long

    /* use memset instead of bzero */
    memset(scheme,'\0',SCHEME_BUFSIZE);
    memset(method,'\0',METHOD_BUFSIZE);
    memset(path,'\0',PATH_BUFSIZE);
    sscanf(buffer, "%s %s %s\r\n\r\n", scheme, method, path);
    gfc->path = path;
    // printf("scheme is %s\n", scheme);
    // printf("method is %s\n", method);
    // printf("path is %s\n", path);

    /* check if the header is valid or not*/
    if (strcmp(scheme, "GETFILE")==0&&strcmp(method, "GET")==0&&strncmp(path,"/",1)==0){
      /* Header is valid, Callback handler, if the handler returns a negative value, send ERROR*/
      if(gfs->handler(gfc, gfc->path, gfs->handlerarg)<0){
        ssize_t return_value = gfs_sendheader(gfc, GF_ERROR, 0); // ERROR
        if (return_value<0){
          fprintf(stderr, "ERROR writing to socket\n");
          exit(1);
        }
      }
    } else {
        /* Check the malformed header */
        ssize_t return_value = gfs_sendheader(gfc, GF_INVALID, 0); // INVALID
        if (return_value<0){
          fprintf(stderr, "ERROR writing to socket\n");
          exit(1);
        }
      }

    /* close the socket connection and free the gfc */
    gfs_abort(gfc);
    free(gfc);
  }
}

void gfserver_set_handlerarg(gfserver_t *gfs, void* arg){
    gfs->handlerarg = arg;
}

void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, const char *, void*)){
    gfs->handler = handler;
}

void gfserver_set_maxpending(gfserver_t *gfs, int max_npending){
    gfs->maxnpending = max_npending;
}

void gfserver_set_port(gfserver_t *gfs, unsigned short port){
    gfs->portno = port;
}
