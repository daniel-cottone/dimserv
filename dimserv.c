/*
 * A very dim web server.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Server configuration
 */
#define SERVER_PORT     22000
#define DOCROOT_DIR     "docroot"
#define VERSION_STRING  "dimserv/0.0.1"

/*
 * Application configuration
 */
#define HEADER_SIZE     10240L
#define BUFFER_SIZE     10240L

int main(int argc, char ** argv)
{
    /* Connection variables */
    char recv_header[HEADER_SIZE], send_header[HEADER_SIZE];
    int listen_fd, comm_fd;
    struct sockaddr_in servaddr;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    listen(listen_fd, 10);

    comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

    while(1)
    {
        /* Initialize buffers */
        memset(recv_header, 0, HEADER_SIZE);
        memset(send_header, 0, HEADER_SIZE);

        read(comm_fd, recv_header, HEADER_SIZE);

        printf("Received request - %s", recv_header);

        char file_buffer[BUFFER_SIZE];

        memset(file_buffer, 0, BUFFER_SIZE);

        FILE *fp;

        fp = fopen(DOCROOT_DIR "/index.html", "r");

        fread(file_buffer, BUFFER_SIZE, 1, fp);

        sprintf(send_header, "HTTP/1.1 OK\r\n"
                             "Server: " VERSION_STRING "\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: %zu\r\n"
                             "\r\n"
                             "%s\r\n", strlen(file_buffer), file_buffer);

        write(comm_fd, send_header, strlen(send_header)+1);

    }
}
