#include "cs537.h"
#include "request.h"
#include <pthread.h>

#if 0
#define my_printf(str, ...) \
  printf(str, ##__VA_ARGS__)
#else
#define my_printf(str, ...)
#endif
//
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


// CS537: Parse the new arguments too
typedef struct connection_information {
  int conn_fd; // connection file descriptor.
  int filled;
  int issued; // has been given to any thread for consumption ?
} conn_info_t;

typedef struct shared_buffer {
  conn_info_t *buffer;
  int fill_count;
} shared_buf_t;

int port = 0, n_threads = 0, n_buffers = 0;
/* All these threads are dynamically allocated */
pthread_t **cnsmr_threads;
pthread_mutex_t main_lock;
shared_buf_t shared_buffer;
pthread_cond_t full, empty;

int put_buffer() {
  int i = 0, index = -1;
    for (i = 0; i < n_buffers; ++i) {
        if (shared_buffer.buffer[i].filled == 0) {
          shared_buffer.buffer[i].filled = 1;
          shared_buffer.fill_count++;
          index = i;
          break;
        }
    }
  return index;
}

int get_buffer() {
  int index = -1, i = 0;
    for (i = 0; i < n_buffers; ++i) {
        if (shared_buffer.buffer[i].filled == 1 &&
        shared_buffer.buffer[i].issued == 0 ) {
          shared_buffer.buffer[i].issued = 1;
          index = i;
          break;
        }
    }
  return index;
}

void free_buffer(int i) {
      shared_buffer.buffer[i].issued = 0;
      shared_buffer.buffer[i].filled = 0;
      shared_buffer.fill_count--;
}

void* connection_consumer(void *arg) {
  while (1) {
    int i = 0;
    int connfd = -1;
    pthread_mutex_lock(&main_lock);
    {
      while (shared_buffer.fill_count == 0) {
        pthread_cond_wait(&full, &main_lock);
      }
      i = get_buffer();
      connfd = shared_buffer.buffer[i].conn_fd;
      if (i == -1) {
        perror("Invalid state of Producer-Consumer relationship ! i = -1");
        exit(1);
      }
      my_printf("Thread %p consumed buffer %d\n", arg, i);
      free_buffer(i);
      pthread_cond_signal(&empty);
    }
    pthread_mutex_unlock(&main_lock);
      
    requestHandle(connfd);
    Close(connfd);
  }
}

void getargs(int argc, char *argv[], int *port, int *n_threads, int *n_buffers)
{
    if (argc != 4) {
      fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
      exit(1);
    }
    *port = atoi(argv[1]);
    *n_threads = atoi(argv[2]);
    *n_buffers = atoi(argv[3]);

    if (!(*port && *n_threads && *n_buffers)) {
      fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
      exit(1);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    int i = 0;

    getargs(argc, argv, &port, &n_threads, &n_buffers);

    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&full, NULL);

    pthread_mutex_init(&main_lock, NULL);

    cnsmr_threads = (pthread_t **) malloc(sizeof(pthread_t *) * n_threads);
    if (cnsmr_threads == 0) {
      goto exit;
    }

    shared_buffer.buffer = (conn_info_t *)
      malloc(sizeof(conn_info_t) * n_buffers);
    if (shared_buffer.buffer == NULL) {
      perror("goto buffer_error\n");
      goto buffer_error;
    }

    for (i = 0; i < n_buffers; ++i) {
      shared_buffer.buffer[i].conn_fd = -1;
      shared_buffer.buffer[i].filled = 0;
    }

    for (i = 0; i < n_threads; ++i) {
      cnsmr_threads[i] = (pthread_t *) malloc(sizeof(pthread_t));
      if (cnsmr_threads[i]) {
        pthread_create(cnsmr_threads[i], NULL, connection_consumer, NULL);
      } else {
        perror("goto free_cnsmr_threads\n");
        goto free_cnsmr_threads;
      }
    }

    listenfd = Open_listenfd(port);
    while (1) {
      int i = 0;
      clientlen = sizeof(clientaddr);
      connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

      //
      // CS537: In general, don't handle the request in the main thread.
      // Save the relevant info in a buffer and have one of the worker threads
      // do the work.
      //
      // requestHandle(connfd);
      pthread_mutex_lock(&main_lock);
      {
        while (shared_buffer.fill_count == n_buffers) {
          pthread_cond_wait(&empty, &main_lock);
        }
        i = put_buffer();
        if (i == -1) {
          perror("Invalid state of Producer-Consumer relationship ! i = -1\n");
          exit(1);
        }

        shared_buffer.buffer[i].conn_fd = connfd;
        my_printf("main Thread produced buffer %d\n", i);
        
        pthread_cond_signal(&full);
      }
      pthread_mutex_unlock(&main_lock);
    }
    return 0;

free_cnsmr_threads:
    i--;
    for (; i>= 0; --i) {
      free(cnsmr_threads[i]);
    }
    free(cnsmr_threads);
buffer_error:
    for (i = 0; i < n_threads; ++i) {
      free(cnsmr_threads[i]);
    }
exit:
    return 1;
}





