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
/* @brief Function to print out usage_and_exit of 
 * this program in case of invalid args
 */
void usage_and_exit() {
  stats_printf("usage_and_exit: stats_client -k key -p priority"
    " -s sleeptime_ns -c cputime_ns\n");
  exit(1);
}

int shmid;
stats_t *shm;
char sem_key[102] = {0};

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
  /* Assuming resonable defaults of priority 1, sleep & Cpu of 1 second */
  unsigned int priority = 1, sleeptime = 1000000000, cputime = 1000000000;
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
  if (statistics == NULL) {
    stats_perror("Unable to connect ! Clients' limit exceeded!\n");
    exit(1);
  }
  statistics->priority = priority;
  statistics->counter = 1;
  strncpy(statistics->argv, argv[0], 15);
  statistics->modified = 1;
  // printf("Modified request for %d %d %d %d\n",
  // statistics->pid, priority, sleeptime, cputime);
  // fflush(stdout);
  /* Handle the ctrl+c interrupt */
  signal(SIGINT, sigint_handler);
  signal(SIGKILL, sigint_handler);

  unsigned int count = 1;
  struct timespec till, left, start, now, absStart, duration;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &absStart);
  for (;;) {
    int rc;
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
    till.tv_sec = sleeptime / SEC_IN_NS;
    till.tv_nsec = sleeptime % SEC_IN_NS;
    rc = nanosleep(&till, &left);
    while (rc < 0) {
      till = left;
      rc = nanosleep(&till, &left);
    }
    count++;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
    float elapsed = timeSpecToFloat(&now) - timeSpecToFloat(&absStart);
    statistics->counter = count;
    statistics->cpu_secs = elapsed;
    // statistics->modified = 1;

    if (2 == count &&
      setpriority(PRIO_PROCESS, statistics->pid, statistics->priority) < 0) {
      stats_perror("setpriority");
    }
  }
  return 0;
}
