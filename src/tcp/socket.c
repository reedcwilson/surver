/*******************************************************************************
 * The interface for the TCP socket API
 ******************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <semaphore.h>

#ifndef MAIN_HEADER
#define MAIN_HEADER
#include "main.h"
#endif

// the port number.
int _port_num = 5000;
// the number of backlog connections
int _backlog_connections_num = 5;
// the number of execution thread
int _num_threads = 10;

// the listen file descriptor
int _listenfd = 0;


/**
 * prints the error message and quits
 */
void error(char *msg, int should_exit) {
  perror(msg);
  fflush(stdout);
  if (exit) {
    exit(1);
  }
}

void usage(void) {
  fprintf(stderr,
      " usage:\n"
      "    ./server [-p portnum] [-b backlog_conns] [-t numthreads]\n"
      "       -p  portnum:        use portnum as the listen port for server\n"
      "       -b: backlog_conns   the number of backlog connections\n"
      "       -t: numthreads      the number of execution threads\n"
      "       -h:                 print out this help message\n"
      "\n");
}

/**
 * parses the command line arguments
 */
void parse_arguments(int argc, char *argv[]) {
  opterr = 0;
  int c;

  while ((c = getopt (argc, argv, "p:b:t:h")) != -1) {
    switch (c) {
      case 'p':
        _port_num = atoi(optarg);
        break;
      case 'b':
        _backlog_connections_num = atoi(optarg);
        break;
      case 't':
        _num_threads = atoi(optarg);
        break;
      case 'h':
        usage();
        exit(1);
      case '?':
        if (optopt == 'p') {
          fprintf (stderr, "Option -%c the port to run on.\n", optopt);
        } else if (optopt == 'b') {
          fprintf (stderr, "Option -%c the number of backlog connections.\n", optopt);
        } else if (optopt == 'h') {
          fprintf (stderr, "Option -%c displays the help list.\n", optopt);
        } else if (optopt == 't') {
          fprintf (stderr, "Option -%c sets the number of execution threads.\n", optopt);
        } else {
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        }
      default:
        usage();
        exit(1);
    }
  }
}

/**
 * get the string representation of a request
 */
char *get_request(int client) {
  static char buffer[BUFFER_SIZE+1];
  char *request = "this is a test";
  int nread;
  while (1) {
    nread = recv(client, buffer, BUFFER_SIZE, 0);
    if (nread < 0) {
      if (errno == EINTR) {
        // the socket call was interrupted -- try again
        continue;
      }
      else {
        // an error occurred, so break out
        return "";
      }
    } else if (nread == 0) {
      // the socket is closed
      return "";
    }
    // TODO: parse the buffer
    request = buffer;
    break;
    // find the end of a request and leave the rest
  }
  printf("%s\n", request);
  return request;
}

/**
 * creates an error response
 */
char *create_error_response(int code, char *message) {
  return "HTTP/1.1 400 Not Found\r\n\r\n";
}

/**
 * attempts to handle the request.
 * sets the value of response
 * returns TRUE(1) if handled FALSE(0) if not
 */
int handle_request(char *request, char *response) {
  return FALSE;
}

/**
 * The main logic for handling requests
 */
void handle(int client) {
  char *response;

  // read a request
  char *request = get_request(client);

  // attempt to handle the request
  if (!handle_request(request, response)) {
    response = create_error_response(404, "Not found");
  }

  // write the response
  if (!write(client, response, strlen(response))) {
    error("error writing to socket", FALSE);
  }

  printf("fd: %d\n", client);
  fflush(stdout);

  close(client);
}

/**
 * An accept thread. This will continue to accept connections until the server 
 * shuts down.
 */
void *serve(void *vptr) {
  int connectionfd, index;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  while ((connectionfd = accept(_listenfd, (struct sockaddr*)&client_addr, &client_len)) > 0) {
    if (connectionfd < 0) {
      error("error on accept", FALSE);
      break;
    }
    handle(connectionfd);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int thread_i = 0, connectionfd = 0;
  struct sockaddr_in serv_addr; 
  pthread_t threads[_num_threads];

  // get the command line arguments
  parse_arguments(argc, argv);

  // create a socket
  _listenfd = socket(AF_INET, SOCK_STREAM, 0);

  // initialize the server_addr and sendBuff
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(_port_num); 

  // bind to the socket
  if (bind(_listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
    error("Failed to bind to socket", TRUE);
  }

  // open shop
  listen(_listenfd, _backlog_connections_num); 

  printf("listening on port %d...\n", _port_num);
  fflush(stdout);

  // the parameters to pass to threads
  char *nothing = "";

  // start the threads
  for (thread_i = 0; thread_i < _num_threads; ++thread_i) {
    pthread_create(&threads[thread_i], NULL, &serve, &nothing);
  }

  // join the threads
  for (thread_i = 0; thread_i < _num_threads; ++thread_i) {
    if (pthread_join(threads[thread_i], NULL)) {
      error("failure to join thread", TRUE);
    }
  }

  // close the socket when finished
  close(_listenfd);
}
