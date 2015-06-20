/*
 * A very dim web server.
 */
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Server configuration
 */
#define SERVER_PORT     22000
#define DOCROOT_DIR     "docroot"
#define MIME_CONFIG     "mime.conf"
#define VERSION_STRING  "dimserv/0.0.1"

/*
 * Application configuration
 */
#define HEADER_SIZE     10240L
#define BUFFER_SIZE     10240L
#define LINE_SIZE       1024L
#define MAX_MIME_TYPES  64L

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

int _true = 1;
int _false = 0;
char * mime_types[MAX_MIME_TYPES][2];

/*
 * Process received headers and return a recv_header_t
 */
recv_header_t * process_recv_header(char * recv_header_buffer) {

  // Return header, token, and token index
  recv_header_t* h = (recv_header_t *) malloc(sizeof(recv_header_t));
  char * token;
  int pos = 0;

  // Process header using pos as token index
  token = strtok(recv_header_buffer, " \r\n");
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

    // Move token index and get next token
    pos = pos+1;
    token = strtok(NULL, " \r\n");
  }

  // Return the processed header
  return h;
}

/*
 * Generate a send_header_t
 */
send_header_t * generate_send_header(char * status, char * content_type, char * content_length) {

  // Return header
  send_header_t* h = (send_header_t *) malloc(sizeof(send_header_t));

  // Set header information
  h->status = status;
  h->server = VERSION_STRING;
  h->content_type = content_type;
  h->content_length = content_length;

  return h;
}

/*
 * Generate MIME type from file extension
 */
char * get_mime_type(char * filename) {

  char * mime_type;
  char * ext;

  if ((ext = strstr(filename, ".")) == NULL) {
    mime_type = "text/unknown";
  } else {
    // Iterate through mime_types list
    for (int i=0; mime_types[i][0] != NULL; i++) {
      if (!strcmp(ext, mime_types[i][0])) {
        mime_type = mime_types[i][1];
      }
    }
  }

  return mime_type;
}

/*
 * Load MIME types from configuration file
 */
int load_mime_types() {

  // Open MIME configuration file
  FILE * fp;
  fp = fopen(MIME_CONFIG, "rb");
  char line[LINE_SIZE];
  int line_count = 0;

  // Return error if unable to open file
  if (!fp) {
    return -1;
  } else {

    // Read file into mime_types
    while (fgets(line, sizeof(line), fp) != NULL) {
      char * token;
      token = strtok(line, " \t\r\n");
      mime_types[line_count][0] = strdup(token);
      token = strtok(NULL, " \t\r\n");
      mime_types[line_count][1] = strdup(token);
      line_count++;
    }
  }

  return 1;
}

/*
 * Handle server shutdown
 */
void shutdown_handler(int sig) {
  printf("[info] Shutting down.\r\n");
  exit(sig);
}

/*
 * Handle client request
 */
void * request_handler(void * socket_desc) {

  /* Create socket from socket descriptor */
  int sock = *(int*)socket_desc;

  /* Initialize buffers and structures */
  char recv_header_buffer[HEADER_SIZE], send_header_buffer[HEADER_SIZE];
  recv_header_t * recv_header;
  send_header_t * send_header;
  memset(recv_header_buffer, 0, HEADER_SIZE);
  memset(send_header_buffer, 0, HEADER_SIZE);
  memset(&recv_header, 0, sizeof(recv_header));
  memset(&send_header, 0, sizeof(send_header));

  /* Get data from socket */
  char buffer[HEADER_SIZE];
  FILE * sock_stream;
  sock_stream = fdopen(sock, "r+");
  while(!feof(sock_stream)) {
    char * in = fgets(buffer, HEADER_SIZE - 2, sock_stream);
    strcat(recv_header_buffer, buffer);

    /* Check for EOF */
    if (!in) {
      break;
    }

    /* Check for end of headers */
    if (!strcmp(in, "\r\n") || !strcmp(in,"\n")) {
      break;
    }
  }

  /* Process raw header from socket */
  recv_header = process_recv_header(recv_header_buffer);
  printf("[info] Received request: %s\r\n", recv_header->filename);

  /* Get MIME type, initialize status */
  char * mime_type = get_mime_type(recv_header->filename);
  char * status = malloc(sizeof(char) * LINE_SIZE);
  char * length = malloc(sizeof(char) * LINE_SIZE);

  /* Get the relative file path */
  char * file_path = malloc(sizeof(char) * (strlen(DOCROOT_DIR) + strlen(recv_header->filename) + 2));
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
    printf("[error] Could not open file: %s\r\n", recv_header->filename);
    fp = fopen(DOCROOT_DIR "/404.html", "rb");
    sprintf(status, "%s", "404 File Not Found");
    sprintf(mime_type, "%s", "text/html");

  } else {

    /* Serve up a 200 */
    sprintf(status, "%s", "200 OK");

  }

  /* Get size of file */
  fseek(fp, 0L, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  sprintf(length, "%lu", size);

  /* Generate a send header */
  send_header = generate_send_header(status, mime_type, length);

  /* Format and send header to socket */
  sprintf(send_header_buffer, "HTTP/1.1 %s\r\n"
                              "Server: " VERSION_STRING "\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %s\r\n"
                              "\r\n", send_header->status, send_header->content_type, send_header->content_length);

  write(sock, send_header_buffer, strlen(send_header_buffer));

  /* Read file and send data to socket */
  fflush(stdout);
  while (!feof(fp)) {
    size_t read_size = fread(file_buffer, 1, BUFFER_SIZE-1, fp);
    write(sock, file_buffer, read_size);
    memset(file_buffer, 0, BUFFER_SIZE);
  }

  /* Send final CR to socket */
  write(sock, "\r\n", strlen("\r\n"));
  fclose(fp);
  fflush(stdout);

  /* Cleanup */
  free(recv_header);
  free(send_header);
  free(status);
  free(length);
  free(file_path);

  /* Free socket descriptor and exit */
  free(socket_desc);
  return 0;
}

/*
 * Main program logic
 */
int main(int argc, char ** argv) {

  /* Server variables */
  int server_port = SERVER_PORT;
  int listen_fd;
  struct sockaddr_in servaddr;
  struct stat _stat;

  /* Initialize socket structures */
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(server_port);

  /* Create socket */
  printf("[info] Creating socket...\r\n");
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    printf("[fatal] Unable to create socket\r\n");
    return -1;
  }
  printf("[info] Socket created successfully.\r\n");

  /* Set socket re-use option */
  printf("[info] Setting socket options...\r\n");
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &_true, sizeof(int)) < 0) {
    printf("[fatal] Could not set socket option: SO_REUSEADDR\r\n");
    close(listen_fd);
    return -1;
  }
  printf("[info] Socket options set successfully.\r\n");

  /* Bind socket to port */
  printf("[info] Binding socket...\r\n");
  if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
    printf("[fatal] Could not bind socket to port: %d\r\n", server_port);
    return -1;
  }
  printf("[info] Socket bound successfully.\r\n");

  /* Check DOCROOT_DIR is accessible */
  printf("[info] Checking service environment...\r\n");
  if (stat(DOCROOT_DIR, &_stat) != 0 || !(S_ISDIR(_stat.st_mode))) {
    printf("[fatal] Could not open DOCROOT_DIR: " DOCROOT_DIR "\r\n");
    return -1;
  }

  /* Load MIME types from configuration file */
  if (load_mime_types() != 1) {
    printf("[fatal] Could not load MIME types from configuration file: " MIME_CONFIG "\r\n");
    return -1;
  }

  /* Listen for incoming connections */
  printf("--------------------------------------------------\r\n");
  printf("[info] Server version: " VERSION_STRING "\r\n");
  printf("[info] Document root: " DOCROOT_DIR "\r\n");
  printf("[info] Listening on port: %d\r\n", server_port);
  listen(listen_fd, 50);

  /* Handle server shutdown */
  signal(SIGINT, shutdown_handler);

  /* Accept incoming connections and pass them to a new thread */
  while(1) {
    int comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL);

    /* Create thread to handle new connection */
    pthread_t thread;
    int * thread_sock = malloc(1);
    *thread_sock = comm_fd;
    if (pthread_create(&thread, NULL, request_handler, (void*) thread_sock) < 0) {
      printf("[fatal] Could not create thread.\r\n");
      return -1;
    }

    /* Join thread to prevent early termination */
    pthread_join(thread, NULL);
  }
}
