#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#define BUFSIZE 630
#define SEEK_SET 0

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: 6200.txt)\n"   \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 30605)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

int main(int argc, char **argv)
{
    int option_char;
    int portno = 30605;             /* port to listen on */
    char *filename = "6200.txt";   /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "xp:hf:", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'f': // file to transfer
            filename = optarg;
            break;
        }
    }


    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    
    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    // Initializing variables
    struct sockaddr_in6 transferServer, transferClient;
    socklen_t addr_size = sizeof transferClient;
    int server_sock_fd, client_sock_fd, data_len_out, optval = 1;
    char data[BUFSIZE];
    FILE * fp;

    // SOCKET()
    if ((server_sock_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed: ");
        exit(1);
    }

    // Setsockopt() to reuse address and avoid "Address already in use" issue
    // Found in Beej's networking guide for resolving this issue
    if ((setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval)) < 0) {
        perror("setsockopt() failed: ");
        exit(-1);
    }

    // Populating server sockaddr_in struct
    memset(&transferServer, 0, sizeof(transferServer));
    transferServer.sin6_family = AF_INET6;
    transferServer.sin6_port = htons(portno);
    transferServer.sin6_addr = in6addr_any;

    // BIND()
    if (bind(server_sock_fd, (struct sockaddr *)&transferServer, sizeof(transferServer)) < 0) {
        perror("bind() failed: ");
        exit(1);
    }

    // LISTEN()
    if (listen(server_sock_fd, 10) < 0) {
        perror("listen() failed: ");
        exit(1);
    }
    
    // Keeping Server connection open - Not sure if efficient/safe
    while (1) {
        // ACCEPT()
        if((client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&transferClient, &addr_size)) < 0) {
        perror("accept() failed: ");
        exit(1);
        }

        // Opening file from Server
        fp = fopen(filename, "r");
        if (fp == NULL) {
            perror("Reading file failed: ");
            exit(1);
        }
        fseek(fp, 0, SEEK_SET);

        // While the file still has contents from fread
        while ((data_len_out = fread(&data, 1, BUFSIZE, fp)) > 0) {

            // SEND()
            if ((send(client_sock_fd, data, data_len_out, 0)) < 0 ) {
                perror("send() failed: ");
                exit(1);
            }

            // Resetting the data variable buffer
            memset(data, 0, BUFSIZE);
        }

        close(client_sock_fd);

    }

    // Freeing up resources
    fclose(fp);
    close(server_sock_fd);
    exit(0);

}