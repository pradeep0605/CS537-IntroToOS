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
#include <signal.h>

// @brief Function to print out usage of this program in case of invalid args
void usage_and_exit() {
  stats_printf("Usage: stats_server -k key\n");
  exit(1);
}

char sem_key[100] = {0};
int shmid = 0;
sem_t *clnt_srvr_sem;
unsigned int key = 0;
stats_t *shm = NULL;

void sigint_handler(int signal) {
  int ret = 0, i = 0;
  if (shmid == 1)
    goto exit;

  /* When Server is exiting, make all the client not in use. */
  for (i = 0; i < 16; i++) {
    shm[i].in_use = 0;
  }

  if ((ret = shmdt(shm)) == -1) {
    stats_perror("shmdt\n");
  }

  /* Remove the attached shared memory */
  if ((ret = shmctl(shmid, IPC_RMID, NULL)) == -1) {
    stats_perror("shmctl\n");
    goto exit;
  }
  sprintf(sem_key, "%s%d", TEAM_NAME, key);
  /* Unlink the named semaphore */
  if ((ret = sem_unlink(sem_key)) == -1) {
    stats_perror("sem_unlink\n");
    goto exit;
  }
exit:
  exit(ret);
}

int
main(int argc, char* argv[]) {
  if (argc > 3) usage_and_exit();
  opterr = 0;
  char c;
  while (-1 != (c = getopt(argc, argv, "k:"))) {
    switch (c) {
      case 'k':
        key = atoi(optarg);
        break;
      default:
        usage_and_exit();
    }
  }
  if (key == 0) {
    usage_and_exit();
  }


  if ((shmid =
    shmget(key, 16 * sizeof(stats_t), IPC_CREAT | IPC_EXCL | 0666)) < 0) {
    stats_perror("Error: %d might already exist !\n", key);
    exit(1);
  }

  if ((shm = (stats_t*) shmat(shmid, NULL, 0)) == (void*)-1) {
    stats_perror("shmat");
    exit(1);
  }

  sprintf(sem_key, "%s%d", TEAM_NAME, key);
  /* Create a semaphore with initial value 0 to make clients wait */
  if ((clnt_srvr_sem =
    sem_open(sem_key, O_CREAT, S_IRUSR | S_IWUSR, 0)) == NULL) {
    stats_perror("Sem_open Failed\n");
  }

  /* To make sure that client do not fail accessing the not-yet created shared
   * memory.
   */
  sem_post(clnt_srvr_sem);
  /* Handle the ctrl+c interrupt and also killed from kill command */
  signal(SIGINT, sigint_handler);
  signal(SIGKILL, sigint_handler);

  unsigned int iter = 1;
  unsigned int any_client_processed = 0;

  for (;;) {
    sleep(1);
    int i;
    for (i = 0; i < 16; i++) {
      if (1 == shm[i].in_use) {
        stats_printf("%d %d %15s %d %.2f %d\n", iter, shm[i].pid, shm[i].argv,
          shm[i].counter, shm[i].cpu_secs, shm[i].priority);
        any_client_processed = true;
      }
    }

    if (any_client_processed) {
      iter++;
      /* To have the extra blank line after each second */
      stats_printf("\n");
      any_client_processed = false;
    }
  }
  return 0;
}
