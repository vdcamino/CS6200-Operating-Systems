#include "gfclient-student.h"
#include <arpa/inet.h>
#include <stddef.h>
#define BUFSIZE 630
#define BUFFSIZE 2080
#define PATH_BUFFER_SIZE 2080
#define MAX_REQUEST_LEN 128
#define DATA_HEADER_BUFFER 5000

// Defining the gfrequest struct
struct gfcrequest_t {
    int server_sock_fd;
    void *headerarg;
    void (*headerfunc)(void*, size_t, void *);
    const char* path;
    unsigned short port;
    const char* server;
    void *writearg;
    void (*writefunc)(void*, size_t, void *);
    size_t bytesreceived;
    size_t filelen;
    gfstatus_t status;
};

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr){
    close((*gfr)->server_sock_fd);
    free(*gfr);
    *gfr = NULL;
}

// Initializing gfrequest and setting some default values to be used later
gfcrequest_t *gfc_create(){  

    gfcrequest_t * gfrequest = NULL;
    gfrequest = (gfcrequest_t *)malloc(sizeof(gfcrequest_t));

    // Defaults
    gfrequest->filelen = 0;
    gfrequest->bytesreceived = 0;
    gfrequest->status = GF_INVALID;

    return gfrequest;
}

/* Getter functions */
size_t gfc_get_bytesreceived(gfcrequest_t **gfr){
    return (*gfr)->bytesreceived;
}

size_t gfc_get_filelen(gfcrequest_t **gfr){
    return (*gfr)->filelen;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr){
    return (*gfr)->status;
}

void gfc_global_init(){
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t **gfr){

    /* Socket Code Here */
    // Converting portno to char * for getaddrinfo()
    char * port;
    port = (char*)malloc(10*sizeof(char));                                          //////////////////////// MALLOC MEMORY +++++++++++++++++++++++++++++++++
    sprintf(port, "%d", (*gfr)->port);
    // Initializing variables
    struct addrinfo hints, *res;                                                    //////////////////////// ADDRINFO MEMORY +++++++++++++++++++++++++++++++++
    struct in6_addr server_addr;
    int ip_version, conn;
    char requestHeader[MAX_REQUEST_LEN];
    memset(requestHeader, 0, MAX_REQUEST_LEN);
    // Header Out variables
    int header_out_total = 0, header_out_buffer = 0;
    // Header In variables
    int header_in_buffer = 0, header_in_total = 0;
    char header_in[DATA_HEADER_BUFFER];
    char header_buffer[DATA_HEADER_BUFFER];
    memset(header_in, 0, DATA_HEADER_BUFFER);
    memset(header_buffer, 0, DATA_HEADER_BUFFER);
    // Data In variables
    int data_in_buffer = 0;
    char data_buffer[DATA_HEADER_BUFFER];
    memset(data_buffer, 0, DATA_HEADER_BUFFER);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype =  SOCK_STREAM;

    // Using inet_pton() to check if the hostname is IPV6 or IPV4
    // Referenced inet_pton from Beej's guide & man pages to get AI_FAMILY type
    ip_version = inet_pton(AF_INET, (*gfr)->server, &server_addr);
    if (ip_version == 1) {
        hints.ai_family = AF_INET;
        printf("%s", "AF_INET");
    } else {
        ip_version = inet_pton(AF_INET6, (*gfr)->server, &server_addr);
        if (ip_version == 1) {
            hints.ai_family = AF_INET6;
            printf("%s", "AF_INET6");
        }
    }

    // Using getaddrinfo to populate the addrinfo struct
    if (getaddrinfo((*gfr)->server, port, &hints, &res) < 0) {
        perror("getaddrinfo() failed: ");
        freeaddrinfo(res);
        (*gfr)->status = GF_ERROR;
        exit(1);
    }

    free(port);                                                                     //////////////////////// Port MALLOC FREED as not used after this  ---------------------------------
    port = NULL;

    // SOCKET()
    if (((*gfr)->server_sock_fd = socket(res->ai_family, res->ai_socktype, 0)) < 0) {
        (*gfr)->status = GF_ERROR;
        freeaddrinfo(res);
        perror("socket() failed: ");
        return 0;
    }

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    // Setsockopt() to reuse address and avoid "Address already in use" issue
    // Found in Beej's networking guide for resolving this issue
    if ((setsockopt((*gfr)->server_sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout))) < 0) {
        (*gfr)->status = GF_ERROR;
        freeaddrinfo(res);
        perror("setsockopt() failed: ");
        return 0;
    }

    // CONNECT()
    if ((conn = connect((*gfr)->server_sock_fd, res->ai_addr, res->ai_addrlen)) < 0) {
        (*gfr)->status = GF_ERROR;
        freeaddrinfo(res);
        perror("connect() failed: ");
        return 0;
    }

    sprintf(requestHeader, "GETFILE GET %s\r\n\r\n", (*gfr)->path);

    while (header_out_total < strlen(requestHeader)) {
        //SEND()
        if ((header_out_buffer = send((*gfr)->server_sock_fd, requestHeader, strlen(requestHeader), 0)) < 0 ) {
            (*gfr)->status = GF_ERROR;
            freeaddrinfo(res);
            perror("send() header failed: ");
            return -1;
        }
        header_out_total += header_out_buffer;
    }

    char * responseDelimiter = "\r\n\r\n";
    // Getting only upto \r\n\r\n in the first pass
    while ((strstr(header_in, responseDelimiter) == NULL)) {

        char * scheme = "GETFILE ";
        char * invalid = "GETFILE INVALID";

        // INVALID check and fail
        if ((strlen(header_in) > strlen(scheme) && strstr(header_in, scheme) == NULL) || (strstr(header_in, invalid) != NULL)) {
            return -1;
        }

        if ((header_in_buffer = recv((*gfr)->server_sock_fd, header_buffer, DATA_HEADER_BUFFER, 0)) <= 0) {
            if (header_in_buffer == 0) {
                perror("recv() header pre-maturely closed: ");
            } else {
                (*gfr)->status = GF_ERROR;
                perror("recv() header data failed: ");
            }
            freeaddrinfo(res);
            return -1;
        }

        memcpy(header_in + header_in_total, header_buffer, (size_t)sizeof(header_buffer));
        header_in_total += header_in_buffer;
        memset(header_buffer, 0, DATA_HEADER_BUFFER);
    }

    // Splitting Header and Content buffers on \r\n\r\n
    char * headerPtr;
    char * contentPtr;

    char header_in_copy[DATA_HEADER_BUFFER];
    memcpy(header_in_copy, header_in, (size_t)DATA_HEADER_BUFFER);

    headerPtr = strtok(header_in_copy, responseDelimiter);

    memset(header_in, 0, (size_t)DATA_HEADER_BUFFER);
    memcpy(header_in, headerPtr, (size_t)strlen(headerPtr));

    contentPtr = strtok(NULL, responseDelimiter);
    ptrdiff_t index = contentPtr - header_in_copy;

    size_t spilloverContentSize = (size_t)0;

    if (index < 0 || index > DATA_HEADER_BUFFER) {
        memset(header_in_copy, 0, DATA_HEADER_BUFFER);
    } else {
        spilloverContentSize = (size_t)(DATA_HEADER_BUFFER-index);
        memcpy(data_buffer, &header_in_copy[index], spilloverContentSize);
        memset(header_in_copy, 0, DATA_HEADER_BUFFER);
    }

    /* Ideally should be it's own function but leaving it here for now
    Some Regex magic to validate the header, extract the path, raise errors and send path to the context struct
    */
    printf("%s", header_in);
    regex_t regex;
    size_t maxGroups = 4;
    regmatch_t groups[maxGroups];
    int value, status;
    char *strStatus, *filelen;
    
    value = regcomp(&regex, "^GETFILE (OK|ERROR|FILE_NOT_FOUND|INVALID) ([0-9]+)$|^GETFILE (ERROR|FILE_NOT_FOUND|INVALID)$", REG_EXTENDED);
    if (value==0) {
        status = regexec(&regex, header_in, maxGroups, groups, 0);
        if (status == 0) {
            // Case when Group 3 (ERROR|FILE_NOT_FOUND|INVALID) is found
            if (groups[3].rm_so != -1 && groups[3].rm_eo != -1) {
                int start = groups[3].rm_so;
                int finish = groups[3].rm_eo;
                size_t len = finish - start;
                strStatus = (char*)malloc(sizeof(char)*(len+1));
                strncpy(strStatus, header_in + start, len);
                strStatus[len] = '\0';
                if (strcmp(strStatus, "INVALID") == 0) {
                    (*gfr)->status = GF_INVALID;
                    free(strStatus);
                    strStatus = NULL;
                    regfree(&regex);
                    return -1;
                } else if (strcmp(strStatus, "FILE_NOT_FOUND") == 0) {
                    (*gfr)->status = GF_FILE_NOT_FOUND;
                } else {
                    (*gfr)->status = GF_ERROR;
                }
                free(strStatus);
                strStatus = NULL;                
                regfree(&regex);
                freeaddrinfo(res);
                return 0;
            // GETFILE OK Status condition
            } else {
                // STATUS
                int start = groups[1].rm_so;
                int finish = groups[1].rm_eo;
                size_t len = finish - start;
                strStatus = (char*)malloc(sizeof(char)*(len+1));
                strncpy(strStatus, header_in + start, len);
                strStatus[len] = '\0';
                if (strcmp(strStatus, "OK") == 0) {
                    (*gfr)->status = GF_OK;
                } else if (strcmp(strStatus, "INVALID") == 0) {
                    (*gfr)->status = GF_INVALID;
                } else if (strcmp(strStatus, "FILE_NOT_FOUND") == 0) {
                    (*gfr)->status = GF_FILE_NOT_FOUND;
                } else {
                    (*gfr)->status = GF_ERROR;
                }
                free(strStatus);
                strStatus = NULL;
                // FILE LEN
                start = groups[2].rm_so;
                finish = groups[2].rm_eo;
                len = finish - start;
                filelen = (char*)malloc(sizeof(char)*(len+1));
                strncpy(filelen, header_in + start, len);
                filelen[len] = '\0';
                (*gfr)->filelen = atoi(filelen);
                free(filelen);
                filelen = NULL;                
            }
        } else {
            regfree(&regex);
            freeaddrinfo(res);
            perror("Header regex match not found: ");
            return -1;
        }
    } else {
        (*gfr)->status = GF_ERROR;
        regfree(&regex);
        freeaddrinfo(res);
        perror("Header regex compilation failed: ");
        return -1;
    }
    regfree(&regex);

    //Getting the data after \r\n\r\n
    (*gfr)->writefunc(data_buffer, spilloverContentSize, (*gfr)->writearg);
    (*gfr)->bytesreceived = spilloverContentSize;
    memset(data_buffer, 0, DATA_HEADER_BUFFER);

    while ((*gfr)->bytesreceived < (*gfr)->filelen) {
        if ((data_in_buffer = recv((*gfr)->server_sock_fd, data_buffer, DATA_HEADER_BUFFER, 0)) <= 0) {
            if (data_in_buffer == 0) {
                perror("recv() data pre-maturely closed: ");
            } else {
                (*gfr)->status = GF_ERROR;
                perror("recv() data failed: ");
            }
            memset(data_buffer, 0, BUFFSIZE);
            freeaddrinfo(res);
            return -1;
        }
        (*gfr)->bytesreceived += data_in_buffer;
        (*gfr)->writefunc(data_buffer, (size_t)data_in_buffer, (*gfr)->writearg);
        memset(data_buffer, 0, BUFFSIZE);
    }

    freeaddrinfo(res);
    return 0;
}

void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg){
    (*gfr)->headerarg = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *)){
    (*gfr)->headerfunc = headerfunc;
}

void gfc_set_path(gfcrequest_t **gfr, const char* path){
    (*gfr)->path = path;
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port){
    (*gfr)->port = port;
}

void gfc_set_server(gfcrequest_t **gfr, const char* server){
    (*gfr)->server = server; 
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg){
    (*gfr)->writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *)){
    (*gfr)->writefunc = writefunc;
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