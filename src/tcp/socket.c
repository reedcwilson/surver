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


// prints the error message and quits
void error(char *msg) {
  perror(msg);
  exit(1);
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

// parses the command line arguments
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

void *serve(void *vptr) {
  int connectionfd;
  time_t ticks;
  char send_buff[BUFFER_SIZE];
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  // initialize the buffer
  bzero(send_buff, sizeof(send_buff)); 

  while ((connectionfd = accept(_listenfd, (struct sockaddr*)&client_addr, &client_len)) > 0) {
    if (connectionfd < 0) {
      error("error on accept");
    }
    ticks = time(NULL);
    snprintf(send_buff, sizeof(send_buff), "%.24s\r\n", ctime(&ticks));
    if (!write(connectionfd, send_buff, strlen(send_buff))) {
      error("error writing to socket");
    }

    printf("fd: %d", connectionfd);
    printf("%.24s\n", ctime(&ticks));
    fflush(stdout);

    close(connectionfd);
    sleep(1000);
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
    error("Failed to bind to socket");
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
      error("failure to join thread");
    }
  }

  // close the socket when finished
  close(_listenfd);
}
