#include "stats.h"

stats_t* stats_init(key_t key) {
  stats_t* retval = NULL;
  int shmid;
  if ((shmid = shmget(key, 16*sizeof(stats_t), 0666)) < 0) {
    perror("shmget");
    return retval;
  }
  stats_t* shm;
  if ((shm = (stats_t*)shmat(shmid, NULL, 0)) == (void*)-1) {
    perror("shmat");
    return retval;
  }

  int i;
  for (i = 0; i < 16; i++) {
    if (0 == shm[i].in_use) {
      retval = &(shm[i]);
      break;
    }
  }

  if (NULL != retval) {
    retval->pid = 0;
    retval->counter = 0;
    retval->priority = 0;
    retval->cpu_secs = 0.0;
    retval->in_use = 1;
  }
  return retval;
}

int stats_unlink(key_t key) {
  int shmid;
  int retval = -1;
  if ((shmid = shmget(key, 16*sizeof(stats_t), 0666)) < 0) {
    perror("shmget");
    return retval;
  }
  stats_t* shm;
  if ((shm = (stats_t*)shmat(shmid, NULL, 0)) == (void*)-1) {
    perror("shmat");
    return retval;
  }
  int pid = getpid();
  int i;
  for (i = 0; i < 16; i++) {
    if (1 == shm[i].in_use && pid == shm[i].pid) {
      shm[i].in_use = 0;
      retval = 0;
      break;
    }
  }
  /*TODO(pradeep) also call shmdt*/
  return retval;
}
