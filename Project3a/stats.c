#include "stats.h"

sem_t *clnt_srvr_sem;
extern int shmid;
extern stats_t *shm;
extern char sem_key[100];
stats_t zero;

stats_t* stats_init(key_t key) {
  stats_t* retval = NULL;
  sprintf(sem_key, "%s%d", TEAM_NAME, key);
  /* Try to open an existing semaphore (created by the server. if doesnt exist,
   * then exit./
   */
  if ((clnt_srvr_sem =
    sem_open(sem_key, 0, S_IRUSR | S_IWUSR, 0)) == NULL) {
    /* Server is not running ! so exit */
    exit(0);
  }

  sem_wait(clnt_srvr_sem);

  if ((shmid = shmget(key, 16*sizeof(stats_t), 0666)) < 0) {
    perror("shmget");
    sem_post(clnt_srvr_sem);
    if (sem_close(clnt_srvr_sem) == -1) {
      stats_perror("sem_close\n");
    }
    return retval;
  }
  if ((shm = (stats_t*)shmat(shmid, NULL, 0)) == (void*)-1) {
    perror("shmat");
    sem_post(clnt_srvr_sem);
    if (sem_close(clnt_srvr_sem) == -1) {
      stats_perror("sem_close\n");
    }
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
    retval->pid = getpid();
    retval->counter = 0;
    retval->priority = 0;
    retval->cpu_secs = 0.0;
    retval->argv[0] = '\0';
    retval->in_use = 1;
    retval->modified = 0;
    // printf("Received request for %d at %d\n", retval->pid, i);
    // fflush(stdout);
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
      // printf("Received unlink request for %d\n", pid);
      // fflush(stdout);
      shm[i] = zero; /* Clear the entire shm location */
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
