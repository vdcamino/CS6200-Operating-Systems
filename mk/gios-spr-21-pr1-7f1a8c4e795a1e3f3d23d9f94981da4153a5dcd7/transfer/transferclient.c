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

#define USAGE                                                  \
    "usage:\n"                                                 \
    "  transferclient [options]\n"                             \
    "options:\n"                                               \
    "  -s                  Server (Default: localhost)\n"      \
    "  -p                  Port (Default: 30605)\n"            \
    "  -o                  Output file (Default cs6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 30605;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:xp:o:h", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */
    // Initializing variables
    struct sockaddr_in6 transferServer;
    socklen_t addr_size = sizeof transferServer;
    int server_sock_fd, conn, data_len_in;
    char data[BUFSIZE];
    FILE * fp;

    // SOCKET()
    if ((server_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed: ");
        exit(1);
    }

    // Populating server sockaddr_in struct
    memset(&transferServer, 0, sizeof(transferServer));
    transferServer.sin6_family = AF_INET;
    transferServer.sin6_port = htons(portno);
    transferServer.sin6_addr = in6addr_any;

    // CONNECT()
    if ((conn = connect(server_sock_fd, (struct sockaddr *)&transferServer, addr_size)) < 0) {
        perror("connect() failed: ");
        exit(1);
    }

    // Opening the local file in client
    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Reading local client file failed: ");
        close(server_sock_fd);
        fclose(fp);
        exit(1);
    }
    fseek(fp, 0, SEEK_SET);

    // While the file still has contents from fread
    while ((data_len_in = recv(server_sock_fd, data, BUFSIZE, 0)) > 0) {

        if ((data_len_in < 0 )) {
            perror("send() failed: ");
            exit(1);
        }

        // Write to local file in client
        fwrite(&data, 1, data_len_in, fp);

        // Resetting the data variable buffer
        memset(data, 0, BUFSIZE);
    }

    // Freeing up resources
    close(server_sock_fd);
    fclose(fp);

    exit(0);

}