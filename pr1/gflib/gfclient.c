#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h> 

#include "gfclient-student.h"
#define BUFSIZE 512

 // Modify this file to implement the interface specified in
 // gfclient.h.

// helper function to clear the different buffers we use
void clear_buffer(char *buffer, int bufsize){
  memset(buffer, '\0', bufsize);
}

// helper function to get the return code
int get_return_code(gfstatus_t status, size_t bytes_received, size_t file_length){
  // return code based on status, number of bytes received and file length 
  if (status == GF_ERROR || status == GF_FILE_NOT_FOUND || (status == GF_OK && bytes_received == file_length))
    return 0;
  else
    return -1;
}

// request struct 
struct gfcrequest_t {
  int socket_fd; // file descriptor for socket 
  unsigned short port; // port used to transfer file
  size_t bytes_received; // number of bytes received  
  size_t file_length; // length of the file received from server 
  gfstatus_t status; // enum...
  char *server;
  char *path; // path of the request 
  char *response; // response obtained from the server 
  char *content; // content obtained from the server 
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

  // declare all variables needed to receive the file
  char buffer[BUFSIZE]; // buffer that contains all the data 
  char content[BUFSIZE];
  char header[BUFSIZE];
  int packet_bytes_rcvd = 0;
  bool header_rcvd = false;
  int sscanf_return = 0; // used to check status/file length missing error
  size_t len;

  // response format = <scheme> <status> <length>\r\n\r\n<content>

  // loop to receive file by chunks
  while(1){
    clear_buffer(buffer, BUFSIZE);
    clear_buffer(content, BUFSIZE);
    packet_bytes_rcvd = recv((*gfr)->socket_fd, buffer, BUFSIZE, 0);
    memcpy(content, buffer, packet_bytes_rcvd);
    (*gfr)->response = buffer;

    // check if header is received
    if (header_rcvd == false){
      // check possible errors
      // error 1. no data was transferred (no content error)
      if (packet_bytes_rcvd == 0) {
        (*gfr)->status = GF_ERROR;
        break;
      }
      
      // error 2. header is malformed if it does not contain GETFILE (no scheme) or ‘\r\n\r\n’ (no delimiter)
      if(strstr(buffer, "GETFILE") == NULL || strstr(buffer, "\r\n\r\n") == NULL){
        (*gfr)->status = GF_INVALID;
        break;
      }

      // get the header 
      char *buffer_header = strtok(buffer, "\r\n\r\n");
      if(buffer_header==NULL) {
        (*gfr)->status = GF_INVALID;
        break;
      }
      // get status from the header
      char header_status[16];

      // error 3. missing file length or status 
      sscanf_return = sscanf(buffer_header, "GETFILE %s %zu\r\n\r\n", header_status, &len);
      if(sscanf_return != 2){
        sscanf_return = sscanf(buffer_header, "GETFILE %s\r\n\r\n", header_status); 
      }
      if (sscanf_return == 2){
        header_rcvd = true;
        if(strcmp(header_status, "OK")==0){
          (*gfr)->status = GF_OK; 
          (*gfr)->file_length = len;
        }else{
          (*gfr)->status = GF_INVALID;
          break;
        }
      }
      else if (sscanf_return == 1) {
        header_rcvd = true; 
        if(strcmp(header_status, "FILE_NOT_FOUND")==0){
          (*gfr)->status = GF_FILE_NOT_FOUND;
        } else if (strcmp(header_status, "INVALID")==0){
          (*gfr)->status = GF_INVALID;
        } else{
          (*gfr)->status = GF_ERROR;
        }
        break;
      } else {
        (*gfr)->status = GF_INVALID;
        break;
      }

      // string header
      clear_buffer(header, BUFSIZE);
      sprintf(header, "GETFILE %s %zu\r\n\r\n", header_status, (*gfr)->file_length);
      // update number of bytes received
      (*gfr)->bytes_received += packet_bytes_rcvd - strlen(header);
      // write content if no errors
      if((*gfr)->write_func != NULL && (*gfr)->status == GF_OK)
        (*gfr)->write_func(content+strlen(header), packet_bytes_rcvd - strlen(header), (*gfr)->write_args);
    } else {
      // error where server closed before data transfer was finished
      if (packet_bytes_rcvd == 0 && (*gfr)->bytes_received < len){
        (*gfr)->status = GF_OK;
        break;
      }
      (*gfr)->write_func(content, packet_bytes_rcvd, (*gfr)->write_args);
      (*gfr)->bytes_received += packet_bytes_rcvd;
    }
    if(len == (*gfr)->bytes_received)
      break;
  } 
  return get_return_code((*gfr)->status, (*gfr)->bytes_received, (*gfr)->file_length);
}

void gfc_set_server(gfcrequest_t **gfr, const char *server){
  (*gfr)->server = server; 
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port) {
  (*gfr)->port = port;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void *, size_t, void *)) {
  (*gfr)->write_func = writefunc; 
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg) {
  (*gfr)->write_args = writearg;
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