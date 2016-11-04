#ifndef __STATS_H__
#define __STATS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

char output[1000] = {0};

#define TEAM_NAME "kashtal"
#define true 1
#define false 0
#define MAX_CLIENTS 16
#define stats_printf(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDOUT_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDOUT\n");
                                                           
#define stats_perror(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDERR_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDERR\n");

#define SEC_IN_NS (1000000000)

typedef struct {
  // You may add any new fields that you believe are necessary
  int pid;         // Do not remove or change
  int counter;     // Do not remove or change
  int priority;    // Do not remove or change
  double cpu_secs; // Do not remove or change
  // You may add any new fields that you believe are necessary
  int in_use;
  char argv[16];
  int modified;
} stats_t;

stats_t* stats_init(key_t key);

int stats_unlink(key_t key);

#endif /* __STATS_H__ */
