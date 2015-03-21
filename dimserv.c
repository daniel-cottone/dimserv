/*
 * A very dim web server.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <regex.h>
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

/*
 * Data configuration
 */
typedef struct {
  char * method;
  char * filename;
  char * http_version;
  char * host;
} recv_header_t;

typedef struct {
  char * status;
  char * server;
  char * content_type;
  char * content_length;
} send_header_t;

/*
 * Process received headers and return a recv_header_t
 */
 recv_header_t * process_recv_header(char * recv_header_buffer) {

   // Return header, token, and token index
   recv_header_t* h = (recv_header_t *) malloc(sizeof(recv_header_t));
   char * token;
   int pos = 0;

   /*
   * Process header using pos as token index
   */
   token = strtok(recv_header_buffer, " \n");
   while (token != NULL) {
     switch(pos) {

       // Process method
       case 0:
       if (!strcmp(token, "GET")) {
         h->method = "GET";
       }
       else if (!strcmp(token, "POST")) {
         h->method = "POST";
       }
       break;

       // Process filename
       case 1:
       if (!strcmp(token, "/")) {
         h->filename = "/index.html";
       }
       else {
         h->filename = token;
       }
       break;

       // Process HTTP version
       case 2:
       h->http_version = token;
       break;

       // Process host
       case 4:
       h->host = token;
       break;

       // Default
       default:
       break;

     }

     // DEBUG
     //printf("Token: %s\r\n", token);

     // Move token index and get next token
     pos = pos+1;
     token = strtok(NULL, " \n");
   }

   // Return the processed header
   return h;
 }

 /*
  * Generate a send_header_t
  */
 send_header_t * generate_send_header(char * status, char * content_type) {

   // Return header
   send_header_t* h = (send_header_t *) malloc(sizeof(send_header_t));

   // Set header information
   h->status = status;
   h->server = VERSION_STRING;
   h->content_type = content_type;

   return h;
 }

 /*
  * Generate MIME type from file extension
  */
 char * get_mime_type(char * filename) {

   char * mime_type;
   char * ext = strstr(filename, ".");

   if (!strcmp(ext, ".html")) {
     mime_type = "text/html";
   }
   else if (!strcmp(ext, ".css")) {
     mime_type = "text/css";
   }
   else if (!strcmp(ext, ".ico")) {
     mime_type = "image/x-icon";
   }
   else if (!strcmp(ext, ".png")) {
     mime_type = "image/png";
   }
   else {
     mime_type = "text/unknown";
   }

   return mime_type;
 }

int main(int argc, char ** argv) {

  /* Connection variables */
  char recv_header_buffer[HEADER_SIZE], send_header_buffer[HEADER_SIZE];
  int listen_fd, comm_fd;
  struct sockaddr_in servaddr;
  recv_header_t * recv_header;
  send_header_t * send_header;

  /* Initialize data structures */
  memset(&servaddr, 0, sizeof(servaddr));
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(SERVER_PORT);

  bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  listen(listen_fd, 10);

  comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

  while(1) {

    /* Initialize buffers and structures */
    memset(recv_header_buffer, 0, HEADER_SIZE);
    memset(send_header_buffer, 0, HEADER_SIZE);
    memset(&recv_header, 0, sizeof(recv_header));
    memset(&send_header, 0, sizeof(send_header));

    /* Receive data from socket */
    read(comm_fd, recv_header_buffer, HEADER_SIZE);
    printf("Received request - %s", recv_header_buffer);

    // Process our raw header
    recv_header = process_recv_header(recv_header_buffer);
    //printf("Recv_header data: %s %s %s\r\n", recv_header->method, recv_header->filename, recv_header->http_version);

    // Get our MIME type
    char * mime_type;
    mime_type = get_mime_type(recv_header->filename);

    /* Get the relative file path */
    char * file_path = calloc(sizeof(char) * (strlen(DOCROOT_DIR) + strlen(recv_header->filename) + 2), 1);
    strcat(file_path, DOCROOT_DIR);
    strcat(file_path, recv_header->filename);

    /* Create file buffer, pointer, and open file */
    char file_buffer[BUFFER_SIZE];
    memset(file_buffer, 0, BUFFER_SIZE);
    FILE * fp;
    fp = fopen(file_path, "rb");

    /* Check if the file was opened */
    if (!fp) {

      /* Serve up a 404 */
      printf("Could not open file: %s\r\n", file_path);
      fp = fopen(DOCROOT_DIR "/404.html", "rb");
      send_header = generate_send_header("404 File Not Found", "text/html");

    } else {

      /* Serve up a 200 */
      send_header = generate_send_header("200 OK", mime_type);

    }

    /* Read file and send data to socket */
    fread(file_buffer, BUFFER_SIZE, 1, fp);
    sprintf(send_header_buffer, "HTTP/1.1 %s\r\n"
                                "Server: " VERSION_STRING "\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %zu\r\n"
                                "\r\n"
                                "%s\r\n", send_header->status, send_header->content_type, strlen(file_buffer), file_buffer);

    write(comm_fd, send_header_buffer, strlen(send_header_buffer)+1);

  }
}
