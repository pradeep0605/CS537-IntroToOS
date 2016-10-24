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

// @brief Function to print out usage of this program in case of invalid args
void usage() {
  char* usageStr = \
  "Usage: stats_client -k key -p priority -s sleeptime_ns -c cputime_ns\n";
  write(STDERR_FILENO, usageStr, strlen(usageStr));
  exit(1);
}

float timeSpecToFloat(struct timespec* t) {
  return (t->tv_sec)+(t->tv_nsec)/1000000000.;
}

int
main(int argc, char* argv[]) {
  if (argc > 9) usage();
  unsigned int key, priority = 0, sleeptime, cputime;
  opterr = 0;
  char c;
  while (-1 != (c = getopt(argc, argv, "k:p:s:c:"))) {
    switch (c) {
      case 'k':
        key = atoi(optarg);
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
        usage();
    }
  }
  stats_t* statistics = stats_init(key);
  int pid = getpid();
  int rc = setpriority(PRIO_PROCESS, pid, priority);
  if (rc < 0) {
    perror("setpriority");
  }
  unsigned int count = 0;
  struct timespec till, left, start, now, absStart, duration;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &absStart);
  for (;;) {
    till.tv_sec = sleeptime/1000000000;
    till.tv_nsec = sleeptime%1000000000;
    rc = nanosleep(&till, &left);
    while (rc < 0) {
      till = left;
      rc = nanosleep(&till, &left);
    }
    rc = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    now = start;
    float now_t = timeSpecToFloat(&now), start_t = timeSpecToFloat(&start);
    duration.tv_sec = cputime/1000000000;
    duration.tv_nsec = cputime%1000000000;
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
    statistics->priority = priority;
    statistics->cpu_secs = elapsed;
    strncpy(statistics->argv, argv[0], 15);
  }
  // TODO(pradeep) call stats_unlink after signal handler is installed
  return 0;
}
