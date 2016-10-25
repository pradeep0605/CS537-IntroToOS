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
#include <signal.h>
// @brief Function to print out usage_and_exit of this program in case of invalid args
void usage_and_exit() {
  stats_printf("usage_and_exit: stats_client -k key -p priority"
    " -s sleeptime_ns -c cputime_ns\n");
  exit(1);
}

int shmid;
stats_t *shm;

float timeSpecToFloat(struct timespec* t) {
  return (t->tv_sec)+(t->tv_nsec)/1000000000.;
}

unsigned int key = 0;
void sigint_handler(int signal) {
  if (stats_unlink(key) == -1) {
    stats_perror("Error in unlinking shm\n");
  }
  exit(0);
}

int
main(int argc, char* argv[]) {
  if (argc > 9) usage_and_exit();
  /* Assuming resonable defaults of priority 10, sleep & Cpu of 1 second */
  unsigned int priority = 10, sleeptime = 1000000000, cputime = 1000000000;
  opterr = 0;
  char c;
  while (-1 != (c = getopt(argc, argv, "k:p:s:c:"))) {
    switch (c) {
      case 'k':
        key = atoi(optarg);
        if (key == 0)
          usage_and_exit();
        break;
      case 'p':
        priority = atoi(optarg);
        break;
      case 's':
        sleeptime = atoi(optarg);
        break;
      case 'c':
        cputime = atoi(optarg);
        break;
      default:
        usage_and_exit();
    }
  }
  stats_t* statistics = stats_init(key);
  int pid = getpid();
  int rc = setpriority(PRIO_PROCESS, pid, priority);
  if (rc < 0) {
    stats_perror("setpriority");
  }
  
  /* Handle the ctrl+c interrupt */
  signal(SIGINT, sigint_handler);
  
  unsigned int count = 0;
  struct timespec till, left, start, now, absStart, duration;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &absStart);
  for (;;) {
    till.tv_sec = sleeptime / SEC_IN_NS; 
    till.tv_nsec = sleeptime % SEC_IN_NS;
    rc = nanosleep(&till, &left);
    while (rc < 0) {
      till = left;
      rc = nanosleep(&till, &left);
    }
    rc = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    now = start;
    float now_t = timeSpecToFloat(&now), start_t = timeSpecToFloat(&start);
    duration.tv_sec = cputime / SEC_IN_NS;
    duration.tv_nsec = cputime % SEC_IN_NS;
    float duration_t = timeSpecToFloat(&duration);
    while (now_t < start_t + duration_t) {
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
      now_t = timeSpecToFloat(&now);
    }
    count++;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
    float elapsed = timeSpecToFloat(&now) - timeSpecToFloat(&absStart);
    statistics->pid = pid;
    statistics->counter = count;
    statistics->priority = getpriority(PRIO_PROCESS, pid);
    statistics->cpu_secs = elapsed;
    strncpy(statistics->argv, argv[0], 15);
  }
  return 0;
}
