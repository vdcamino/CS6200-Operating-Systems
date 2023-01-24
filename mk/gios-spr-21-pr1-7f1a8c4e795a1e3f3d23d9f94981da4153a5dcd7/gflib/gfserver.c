#include "gfserver-student.h"
/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */

// Defining the gfserver_t and gfcontext_t structs
// gfserver_t being populated based on gfserver_set_ functions and to hold server socket
struct gfserver_t {
    int server_sock_fd;
    unsigned short portno;
    int max_npending;
    int status;
    gfh_error_t (*handler)(gfcontext_t **, const char *, void*);
    void* args;
};
// gfcontext_t populated based on the elements the client context needs (from warmups + to hold filepath)
struct gfcontext_t {
    char * filepath;
    int client_sock_fd;
    struct sockaddr_in6 * getfileClient;
};

void gfs_abort(gfcontext_t **ctx){
    close((*ctx)->client_sock_fd);
    free(*ctx);
    *ctx = NULL;
}

// Initializing the server socket and setting the gfserver struct
gfserver_t* gfserver_create(){

    gfserver_t * gfserver = NULL;
    gfserver = (gfserver_t *)malloc(sizeof(gfserver_t));

    // SOCKET()
    if (((*gfserver).server_sock_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed: ");
        exit(1);
    }
    return gfserver;
}

// Straight from the warmups to send the data and len
ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){

    int sent;

    // SEND()
    if ((sent = send((*ctx)->client_sock_fd, data, len, 0)) < 0 ) {
        perror("send() failed: ");
        exit(1);
    }

    return sent;
}

// Customizing response header based on the status arg as per requirements
ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){

    char responseHeader[MAX_REQUEST_LEN];
    int sent;
    
    if (status == GF_OK) {
        sprintf(responseHeader, "GETFILE %s %ld\r\n\r\n", "OK", file_len);
    } else if (status == GF_FILE_NOT_FOUND) {
        sprintf(responseHeader, "GETFILE %s \r\n\r\n", "FILE_NOT_FOUND");
    } else if (status == GF_INVALID) {
        sprintf(responseHeader, "GETFILE %s \r\n\r\n", "INVALID");
    } else {
        sprintf(responseHeader, "GETFILE %s \r\n\r\n", "ERROR");
    }

    // SEND()
    if ((sent = send((*ctx)->client_sock_fd, responseHeader, strlen(responseHeader), 0)) < 0 ) {
        perror("sendheader() failed: ");
        exit(1);
    }

    return sent;
}

void gfserver_serve(gfserver_t **gfs){

    /* Socket Code Here */
    // Initializing variables
    struct sockaddr_in6 transferServer;
    int optval = 1;
    char header_in[MAX_REQUEST_LEN];
    char header_buffer[MAX_REQUEST_LEN];
    int header_len_in;

    // Setsockopt() to reuse address and avoid "Address already in use" issue
    // Found in Beej's networking guide for resolving this issue
    if ((setsockopt((*gfs)->server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval)) < 0) {
        perror("setsockopt() failed: ");
        exit(1);
    }

    // Populating server sockaddr_in struct
    memset(&transferServer, 0, sizeof(transferServer));
    transferServer.sin6_family = AF_INET6;
    transferServer.sin6_port = htons((*gfs)->portno);
    transferServer.sin6_addr = in6addr_any;

    // BIND()
    if (bind((*gfs)->server_sock_fd, (struct sockaddr *)&transferServer, sizeof(transferServer)) < 0) {
        perror("bind() failed: ");
        exit(1);
    }

    // LISTEN()
    if (listen((*gfs)->server_sock_fd, (*gfs)->max_npending) < 0) {
        perror("listen() failed: ");
        exit(1);
    }
    
    // Keeping Server connection open - Not sure if efficient/safe
    while (1) {

        // Initializing client context variables set within the loop
        gfcontext_t * gfclient = NULL;
        gfclient = (gfcontext_t *)malloc(sizeof(gfcontext_t));
        socklen_t addr_size = sizeof(gfclient->getfileClient);
        int total_bytes_received = 0, counter = 1;
        char path[MAX_REQUEST_LEN];

        // ACCEPT()
        if((gfclient->client_sock_fd = accept((*gfs)->server_sock_fd, (struct sockaddr *)&(gfclient->getfileClient), &addr_size)) < 0){
            perror("accept() failed: ");
            gfs_abort(&gfclient);
            continue;             
        }

        // Clearing up buffer variables for new requests
        memset(path, 0, MAX_REQUEST_LEN);
        memset(header_in, 0, MAX_REQUEST_LEN);
        memset(header_buffer, 0, MAX_REQUEST_LEN);

        /* As long as \r\n\r\n is not found in header, the loop to fetch header continues
        Safety check added to make sure input header doesn't break header char
        Also keeping track of received bytes to help with that
        */
        // strstr() for contains() check similar in python
        while ((strstr(header_in, "\r\n\r\n") == NULL) && (total_bytes_received < MAX_REQUEST_LEN)) {
            if ((header_len_in = recv(gfclient->client_sock_fd, header_buffer, MAX_REQUEST_LEN, 0)) < 0) {
                perror("recv() header failed: ");
                gfs_abort(&gfclient);
                continue;
            }
            total_bytes_received += header_len_in;
            if (counter == 1) {
                strcpy(header_in, header_buffer);
            } else {
                strcat(header_in, header_buffer);
            }
            counter ++;
            memset(header_buffer, 0, MAX_REQUEST_LEN);
        }

        printf("Serving request: %s", header_in);
        
        /* Ideally should be it's own function but leaving it here for now
        Some Regex magic to validate the header, extract the path, raise errors and send path to the context struct
        */
        regex_t regex;
        size_t maxGroups = 2;
        regmatch_t groups[maxGroups];
        int value, status;
        value = regcomp(&regex, "^GETFILE GET (/.*?)\r\n\r\n$", REG_EXTENDED);
        if (value==0) {
            status = regexec(&regex, header_in, maxGroups, groups, 0);
            if (status == 0) {
                int start = groups[1].rm_so;
                int finish = groups[1].rm_eo;
                strncpy(path, header_in + start, (finish - start));
                gfclient->filepath = path;

            } else {
                perror("Header regex match failed: ");
                gfs_sendheader(&gfclient, GF_INVALID, 0);
                gfs_abort(&gfclient);
                continue;                
            }
        } else {
            perror("Header regex compilation failed: ");
            gfs_sendheader(&gfclient, GF_INVALID, 0);
            gfs_abort(&gfclient);            
            continue;
        }
        regfree(&regex);

        // Invoking the handler to do the magic
        (*gfs)->status = (*gfs)->handler(&gfclient, gfclient->filepath, (*gfs)->args);

    }

    exit(0);

}


// Setting gfserver_t struct values
void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    (*gfs)->args = arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    (*gfs)->handler = handler;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    (*gfs)->max_npending = max_npending;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    (*gfs)->portno = port;
}