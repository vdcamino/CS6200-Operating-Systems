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
     char *request; /* request message from client*/
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
  struct gfserver_t *gfs = (struct gfserver_t*) malloc(sizeof(struct gfserver_t));
  return gfs;
}


ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){
  /* len_scheme + len_status + file_len 18446744073709551616 */
  ssize_t sent_header;
  char header[MAX_REQUEST_LEN];
  memset(header, '\0', MAX_REQUEST_LEN);
  switch (status) {
    case GF_OK: {
      sprintf(header,"GETFILE OK %zu \r\n\r\n", file_len);
      sent_header = send(ctx->newsockfd, header, strlen(header), 0);
      gfs_abort(ctx);
    }
    break;

    case GF_FILE_NOT_FOUND: {
      strcat(header,"GETFILE FILE_NOT_FOUND \r\n\r\n");
      sent_header = send(ctx->newsockfd, header, strlen(header), 0);
      gfs_abort(ctx);
    }
    break;

    case GF_ERROR: {
      strcat(header,"GETFILE ERROR \r\n\r\n");
      sent_header = send(ctx->newsockfd, header, strlen(header), 0);
      gfs_abort(ctx);
    }
    break;

    case GF_INVALID:{
      strcat(header,"GETFILE INVALID \r\n\r\n");
      sent_header = send(ctx->newsockfd, header, strlen(header), 0);
      gfs_abort(ctx);
    }
    break;
  }
  return sent_header;
}

ssize_t gfs_send(gfcontext_t *ctx, const void *data, size_t len){
  ssize_t sent_data = send(ctx->newsockfd, data, len, 0);
  return sent_data;
}


void gfserver_serve(gfserver_t *gfs){
  int sockfd, newsockfd, clilen;
  struct sockaddr_in serv_addr, cli_addr;
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
    newsockfd = accept(sockfd, (struct sockaddr *) & cli_addr, (socklen_t *)&clilen);
    if (newsockfd < 0){
      fprintf(stderr, "ERROR on accept\n");
      exit(1);
    }
    gfcontext_t *gfc = (gfcontext_t *)malloc(sizeof(gfcontext_t));
    gfc->sockfd = sockfd;
    gfc->newsockfd = newsockfd;
    char buffer[MAX_REQUEST_LEN];
    memset(buffer,'\0',MAX_REQUEST_LEN);
    /* Receive the request from the client*/
    ssize_t received = recv(newsockfd, buffer, MAX_REQUEST_LEN, 0);
    if (received == -1){
      fprintf(stderr, "ERROR reading from socket\n");
      exit(1);
    }

    gfc->request = buffer;
    /*
     * Find the file path from the received request
     * Parse the scheme, method, and filepath
     * <Scheme> <Method> <Path> \r\n\r\n
     */
    char *scheme = strtok(buffer, " ");
    char *method = strtok(NULL, " ");
    char *fpath = strtok(NULL, " ");
    /* remove the possible space after the path */
    char *path = strtok(fpath," ");
    if (scheme == NULL || method == NULL || fpath == NULL) {

      ssize_t return_value = gfs_sendheader(gfc, 400, 0);
      if (return_value<0){
        fprintf(stderr, "\nERROR writing to socket\n");
        exit(1);
      }
      // char header[MAX_REQUEST_LEN];
      // memset(header, '\0', MAX_REQUEST_LEN);
      // strcat(header,"GETFILE INVALID \r\n\r\n");
      // ssize_t sent = send(gfc->newsockfd, header, strlen(header), 0);
      // if (sent == -1){
      //   fprintf(stderr, "\nERROR writing to socket\n");
      //   exit(1);
      // }
      // gfs_abort(gfc);
    }
    // char requestedpath[MAX_REQUEST_LEN];
    // memset(requestedpath, '\0', MAX_REQUEST_LEN);
    // if(strtok(NULL, " ") == NULL){
    //   /* NO space between path and marker*/
    //   sscanf(fpath, "%s\r\n\r\n", requestedpath);
    //   //printf("No Space\n");
    // }
    // else{
    //   /* space between path and marker*/
    //   sscanf(fpath, "%s \r\n\r\n", requestedpath);
    //   //printf("Have Space\n");
    // }
    gfc->path = path;

    int is_scheme_valid = strcmp(scheme, "GETFILE");
    int is_method_valid = strcmp(method, "GET");
    int is_path_valid = strncmp(fpath, "/", 1);

    /*Deal with valid header*/
    if(is_scheme_valid==0&&is_method_valid==0&&is_path_valid==0) {
      gfs->handler(gfc, gfc->path, gfs->handlerarg);
    } else {
      /*Deal with malformed header and incomplete header*/
      char header[MAX_REQUEST_LEN];
      memset(header, '\0', MAX_REQUEST_LEN);
      strcat(header,"GETFILE INVALID \r\n\r\n");
      ssize_t sent = send(gfc->newsockfd, header, strlen(header), 0);
      if (sent == -1){
        fprintf(stderr, "\nERROR writing to socket\n");
        exit(1);
      }
      gfs_abort(gfc);
    }
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
