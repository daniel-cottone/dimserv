/*
 * A very dim web server.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Configuration
 */
#define SERVER_PORT     22000
#define VERSION_STRING  "dimserv/0.0.1"

int main(int argc, char ** argv)
{

    char str[100];
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

        bzero(str, 100);

        read(comm_fd,str,100);

        printf("Received request - %s", str);

        char ret[100];

        sprintf(ret, "HTTP/1.1 OK\r\n"
                     "Server: " VERSION_STRING "\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n"
                     "\r\n"
                     "%s\r\n", strlen("Hello, world!"), "Hello, world!");

        write(comm_fd, ret, strlen(ret)+1);

    }
}
