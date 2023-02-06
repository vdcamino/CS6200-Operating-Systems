
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

 // Modify this file to implement the interface specified in
 // gfclient.h.

// request struct 
struct gfcrequest_t {
  int socket_fd; // file descriptor for socket 
  gfstatus_t status; // enum...
  char *server;
  char *path; // path of the request 
  char *response; // response obtained from the server 
  char *content; // content obtained from the server 
  size_t bytes_received; // number of bytes received  
  size_t file_length; // length of the file received from server 
  unsigned short port; // port used to transfer file
  void (*header_func)(void*, size_t, void *); // header function 
  void *header_args; // header arguments 
  void (*write_func)(void*, size_t, void *); // write function 
  void *write_args; // write arguments
};

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr) {
  free(*gfr);
  *gfr = NULL;
}

gfcrequest_t *gfc_create() {
  // initialize memory for request 
  gfcrequest_t *gf_request = (gfcrequest_t *)malloc(sizeof(gfcrequest_t));
  // set everything to NULL
  memset(gf_request, '\0', sizeof(gfcrequest_t)); 
  // set some default values 
  gf_request->file_length = 0;
  gf_request->status = GF_INVALID;
  gf_request->bytes_received = 0;
  return gf_request;
}

/* Getter functions */
size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
    return (*gfr)->bytes_received;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
    return (*gfr)->file_length;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){
    return (*gfr)->status;
}

void gfc_global_init() {}

void gfc_global_cleanup() {}

int gfc_perform(gfcrequest_t **gfr) {

  /* Socket Code Here */
  // create client socket
  if (((*gfr)->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    exit(14); // error checking: failed to create socket

  // create data structure for server connection 
  // based on Beej's "5.1 getaddrinfo() — Prepare to launch!""
  int status;
  struct addrinfo hints, *res, *p;
  memset(&hints, '\0' , sizeof(hints));
  hints.ai_family = AF_UNSPEC; // could have used AF_INET or AF_INET6 to force version. Here it works for both 
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  // line of code to lose the pesky "address already in use" error message (set SO_REUSEADDR on a socket to true (1))
  int yes = 1; // For setsockopt() SO_REUSEADDR, to avoid errors when a port is still being used by another socket
  setsockopt((*gfr)->socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  // convert port number from int to char because getaddrinfo() requires a string as argument
  char port[10];
  sprintf(port, "%u", (*gfr)->port);
  if ((status = getaddrinfo((*gfr)->server, port, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return 2;
  }

  // loop to connect
  for(p = res; p != NULL; p = p->ai_next) {
    if (connect((*gfr)->socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close((*gfr)->socket_fd);
      if (((*gfr)->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit(11); // error checking: failed to create socket
      continue;
    }
    break; // connection success
  }
  freeaddrinfo(res); // free the linked list

  // build request 
  char req_header[256];
  sprintf(req_header, "GETFILE GET %s\r\n\r\n", (*gfr)->path);
  
  // send request 
  ssize_t total_bytes_sent = 0;
  ssize_t packet_bytes_sent;
  while (total_bytes_sent < strlen(req_header)) {
    if ((packet_bytes_sent = send((*gfr)->socket_fd, req_header, strlen(req_header), 0)) < 0 )
      exit(27);
    total_bytes_sent += packet_bytes_sent;
  }

  

  /* workflow
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
      data_recv = recv((*gfr)->socket_fd, recv_buffer, BUFSIZE-1, 0);
      if (data_recv < 0) {
        fprintf(stderr, "ERROR receiving data from socket\n");
        exit(1);
      }
      /* use content_buffer to locate the start of the contents*/
      memcpy(content_buffer,recv_buffer,data_recv);
      total_recv += data_recv;
      (*gfr)->response = recv_buffer;

      // printf("Start Here--------------\n");
      /* if header is not received, go to deal with header */
      if(is_header_received == 0){
        /* server closed socket in header transfer*/
        if (data_recv == 0) {
          fprintf(stderr, "ERROR receiving data from socket\n");
          (*gfr)->status = GF_ERROR;
          break;
        }
        /* malformed header */
        if(strstr(recv_buffer,"GETFILE ")==NULL||strstr(recv_buffer,"\r\n\r\n")==NULL){
          (*gfr)->status = GF_INVALID;
          // return -1;
          break;
        }
        // printf("======recv_buffer is:%s. How many:%zu=========\n",recv_buffer, data_recv);
        char *buffer_header = strtok(recv_buffer, "\r\n\r\n");
        if(buffer_header==NULL) {
          (*gfr)->status = GF_INVALID;
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
            (*gfr)->status = GF_FILE_NOT_FOUND;
          } else if (strcmp(recv_status, "INVALID")==0){
            (*gfr)->status = GF_INVALID;
          } else{
            (*gfr)->status = GF_ERROR;
          }
          break;
        } else if (flag == 2){
          is_header_received = 1;
          if(strcmp(recv_status, "OK")==0){
            // printf("====3rd Status is %s===\n", recv_status);
            (*gfr)->status = GF_OK; //received header with status OK and file length
            (*gfr)->file_length = file_len;
          }else{
            (*gfr)->status = GF_INVALID;
            break;
          }
        } else {
          (*gfr)->status = GF_INVALID; //received header with others, regarding as invalid
          break;
        }


        memset(header, '\0', HEADER_BUFFER_SIZE);
        sprintf(header, "GETFILE %s %zu\r\n\r\n", recv_status, (*gfr)->file_length);
        // printf("=====I generate a header as is %s====\n", header);

        file_recv = file_recv+data_recv-strlen(header);
        (*gfr)->bytes_received = file_recv;
        /* write content after the header*/
        if((*gfr)->write_func!=NULL&&(*gfr)->status == GF_OK) {
          (*gfr)->write_func(content_buffer+strlen(header), data_recv - strlen(header), (*gfr)->write_args);
        }
      } else {
        // printf("====== How many:%zu=======\n", data_recv);
        if (data_recv < 0) {
          fprintf(stderr, "ERROR receiving data from socket\n");
          exit(1);
        }
        /* server closed socket in content transfer*/
        if (data_recv ==0 && file_recv<file_len){
          (*gfr)->status = GF_OK;
          // return -1;
          break;
        }
        /* write content */
        (*gfr)->write_func(content_buffer, data_recv, (*gfr)->write_args);
        (*gfr)->bytes_received = file_recv;
        file_recv = file_recv + data_recv;
      }

    }while(file_len>file_recv); // keep receiving data until file_len==file_recv


    (*gfr)->bytes_received = file_recv;
    return 0;
}



void gfc_set_server(gfcrequest_t **gfr, const char *server){
  (*gfr)->server = server; 
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port) {
  (*gfr)->port = port;
}

void gfc_set_path(gfcrequest_t **gfr, const char *path) {
  (*gfr)->path = path;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg) {
  (*gfr)->header_args = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void *, size_t, void *)) {
  (*gfr)->header_func = headerfunc;
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg) {
  (*gfr)->write_args = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void *, size_t, void *)) {
  (*gfr)->write_func = writefunc; 
}

const char *gfc_strstatus(gfstatus_t status) {
  const char *strstatus = "UNKNOWN";

  switch (status) {

   case GF_ERROR: {
      strstatus = "ERROR";
    } break;

    case GF_OK: {
      strstatus = "OK";
    } break;


   case GF_INVALID: {
      strstatus = "INVALID";
    } break;


    case GF_FILE_NOT_FOUND: {
      strstatus = "FILE_NOT_FOUND";
    } break;
  }

  return strstatus;
}