#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "stats.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// @brief Function to print out usage of this program in case of invalid args
void usage() {
  char* usageStr = \
  "Usage: stats_server -k key\n";
  write(STDERR_FILENO, usageStr, strlen(usageStr));
  exit(1);
}

int
main(int argc, char* argv[]) {
  if (argc > 3) usage();
  unsigned int key;
  opterr = 0;
  char c;
  while (-1 != (c = getopt(argc, argv, "k:"))) {
    switch (c) {
      case 'k':
        key = atoi(optarg);
        break;
      default:
        usage();
    }
  }
  int shmid;
  if ((shmid = shmget(key, 16*sizeof(stats_t), IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    exit(1);
  }
  stats_t *shm = NULL;
  if ((shm = (stats_t*)shmat(shmid, NULL, 0)) == (void*)-1) {
    perror("shmat");
    exit(1);
  }
  unsigned int iter = 1;
  for (;;) {
    sleep(1);
    int i;
    for (i = 0; i < 16; i++) {
      if (1 == shm[i].in_use) {
        fprintf(stdout, "%d %d %s %.2f %d\n",
            iter, shm[i].pid, shm[i].argv, shm[i].cpu_secs, shm[i].priority);
        fflush(stdout);
      }
    }
  }
  // TODO(pradeep) call shmctl(shmid, IPC_RMID, NULL) in signal handler
  return 0;
}
