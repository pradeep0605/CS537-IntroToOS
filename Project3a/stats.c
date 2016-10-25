#include "stats.h"

sem_t *clnt_srvr_sem;
extern int shmid;
extern stats_t *shm;
extern char sem_key[100];

stats_t* stats_init(key_t key) {
  stats_t* retval = NULL;
  sprintf(sem_key, "%d", key);
  if ((clnt_srvr_sem =
    sem_open(sem_key, O_CREAT, S_IRUSR | S_IWUSR, 0)) == NULL) {
    stats_perror("Sem_open Failed\n");
    exit(1);
  }
  
  sem_wait(clnt_srvr_sem);

  if ((shmid = shmget(key, 16*sizeof(stats_t), 0666)) < 0) {
    perror("shmget");
    return retval;
  }
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
  sem_post(clnt_srvr_sem);
  
  return retval;
}

/* Unlink is called when ctrl+c is hit */
int stats_unlink(key_t key) {
  int retval = -1;

    
  sem_wait(clnt_srvr_sem);
  int pid = getpid();
  int i;
  for (i = 0; i < 16; i++) {
    /* Find and free this process's SHM usage */
    if (1 == shm[i].in_use && pid == shm[i].pid) {
      shm[i].in_use = 0;
      retval = 0;
      break;
    }
  }
  
  if ((retval = shmdt(shm)) == -1) {
    stats_perror("shmdt\n");
  }

  sem_post(clnt_srvr_sem);

  if (sem_close(clnt_srvr_sem) == -1) {
    stats_perror("sem_close\n");
  }

  return retval;
}
