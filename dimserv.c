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

#define BUFFER_SIZE     1024

int main(int argc, char ** argv)
{

    char str[BUFFER_SIZE];
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

        bzero(str, BUFFER_SIZE);

        read(comm_fd, str, BUFFER_SIZE);

        printf("Received request - %s", str);

        char ret[BUFFER_SIZE];

        char file_contents[BUFFER_SIZE];

        memset(file_contents, 0, BUFFER_SIZE);

        FILE *fp;

        fp = fopen("docroot/index.html", "r");

        fread(file_contents, BUFFER_SIZE, 1, fp);

        sprintf(ret, "HTTP/1.1 OK\r\n"
                     "Server: " VERSION_STRING "\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n"
                     "\r\n"
                     "%s\r\n", strlen(file_contents), file_contents);

        write(comm_fd, ret, strlen(ret)+1);

    }
}
