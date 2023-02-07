#include "gfserver-student.h"

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */

#define BUFSIZE 512

// server data structure
struct gfserver_t {
    gfh_error_t (*handlerfunc)(gfcontext_t **, const char *, void*);
    unsigned short portno;
    int max_npending; // maximum number of waiting requests in the queue
    void* arg; // handle arg 
};

// context data structure
struct gfcontext_t{
    int client_socket_fd;
    size_t file_len;
 };

// helper function to clear the different buffers we use
void clear_buffer(char *buffer, int bufsize){
  memset(buffer, '\0', bufsize);
}

void gfs_abort(gfcontext_t **ctx){
    close((*ctx)->client_socket_fd);
}

gfserver_t* gfserver_create(){
    gfserver_t* server = (gfserver_t*)malloc(sizeof(gfserver_t));
    return server;
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len) {
    ssize_t total_bytes_sent = 0;
    ssize_t bytes_sent;
    while (total_bytes_sent < len){
        bytes_sent = send((*ctx)->client_socket_fd, data + total_bytes_sent, len - total_bytes_sent, 0);
        if (bytes_sent <= 0)
            break;
        total_bytes_sent += bytes_sent;
    }
    return total_bytes_sent;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    ssize_t total_bytes_sent = 0;
    char ok_res[BUFSIZE];
    switch (status) {
        case GF_FILE_NOT_FOUND:
            total_bytes_sent = send((*ctx)->client_socket_fd, "GETFILE FILE_NOT_FOUND \r\n\r\n", strlen("GETFILE FILE_NOT_FOUND \r\n\r\n"), 0);
            break;
        case GF_OK:
            (*ctx)->file_len = file_len;
            sprintf(ok_res,"GETFILE OK %zu \r\n\r\n", file_len);
            total_bytes_sent = send((*ctx)->client_socket_fd, ok_res, strlen(ok_res), 0);
            break;
        case GF_ERROR:
            total_bytes_sent = send((*ctx)->client_socket_fd, "GETFILE ERROR \r\n\r\n", strlen("GETFILE ERROR \r\n\r\n"), 0);
            break;
    }
    return total_bytes_sent;
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*gfs)->arg = arg;
}

void gfserver_serve(gfserver_t **gfs){
    /* Socket Code Here */
    // create data structure for server connection 
    // based on Beej's "5.1 getaddrinfo() — Prepare to launch!""
    int status;
    int server_socket_fd, client_socket_fd;
    struct addrinfo hints, *res, *p;
    memset(&hints, '\0' , sizeof(hints));
    hints.ai_family = AF_UNSPEC; // could have used AF_INET or AF_INET6 to force version. Here it works for both 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    // line of code to lose the pesky "address already in use" error message (set SO_REUSEADDR on a socket to true (1))
    int yes = 1; // For setsockopt() SO_REUSEADDR, to avoid errors when a port is still being used by another socket

    // convert port number from int to char because getaddrinfo() requires a string as argument
    char port[10];
    sprintf(port, "%u", (*gfs)->portno);
    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    // loop to connect
    for(p = res; p != NULL; p = p->ai_next) {
        if((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            exit(11); // error checking: failed to create socket
        if(setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
            exit(14); // error checking: reuse error
        if(bind(server_socket_fd,p->ai_addr,p->ai_addrlen) < 0)
            continue; // error checking: binding error
        break; // connection success
    }
    freeaddrinfo(res); // free the linked list

    // start listening
	if (listen(server_socket_fd, (*gfs)->max_npending) < 0)
		exit(1); // error checking: failed to make socket start listening
    
    // client + receive/send file in chunks 
    struct sockaddr_storage client_addr;   
    socklen_t client_len;
    size_t bytes_rcvd = 0;
    char request[BUFSIZE];
    char buffer[BUFSIZE];
    
    // infinite loop: server should not terminate after sending its first response; rather, it should prepare to serve another request
    while(1) {
        // new context 
        gfcontext_t *context = malloc(sizeof(gfcontext_t));
        clear_buffer(buffer, BUFSIZE);

        // connect to the client
        client_len = sizeof(client_addr);
        if ((client_socket_fd = accept(server_socket_fd,(struct sockaddr*)&client_addr,&client_len)) < 0)
            exit(27); // error checking: failed to accept connection 
    
        // process the request 
        while(1){
            if (!((bytes_rcvd = recv(client_socket_fd,buffer,BUFSIZE,0)) > 0))
                break; // recv finished or error during transfer

            // general form of a request = <scheme> <method> <path>\r\n\r\n
            char *scheme = strtok(buffer," "); // request scheme
            char *method = strtok(NULL," "); // request method
            char *path = strtok(NULL,"\r\n\r\n"); // request path 

            // checking possible errors that invalidate the request 
            if(strcmp(scheme,"GETFILE") != 0 || strcmp(method,"GET") != 0 || (path != NULL && strcmp(path,"/") != 0)){
                send(client_socket_fd,"GETFILE INVALID\r\n\r\n", 19, 0);
                break;
            }
            (*context).client_socket_fd = client_socket_fd;
            (*gfs)->handlerfunc(&context, path, (*gfs)->arg);
            free(context);
            break;
        }
    }
    free(*gfs);
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*gfs)->handlerfunc = handler;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*gfs)->portno = port;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*gfs)->max_npending = max_npending;
}