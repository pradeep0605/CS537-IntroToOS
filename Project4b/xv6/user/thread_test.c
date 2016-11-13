#include "types.h"
#include "stat.h"
#include "user.h"

#define NLOOP 120
void thread_code(void *arg)
{
  int N =  *(int *)arg;
  int i = 0;
  printf(1, "thread_code : N = %d &i = %d(%d)\n", N, i, &i);
  for (i = 0; i < N; ++i) {
    printf(1, "Thread | i = %d | &i= %d\n", i, &i);
  }
  exit();
}

int main(int argc, char *argv[])
{
  int tid = 0;
  int N = NLOOP;
  
  tid = thread_create(thread_code, (void*) &N);
  /*
  int i = 0;
  for (i = 0; i < NLOOP; ++i) {
    //printf(1, "%s:%d: %d\n", __func__, tid, i);
  }
  */
  printf(1, "Parent Sleeping\n");
  thread_join(tid);
  printf(1, "Sleeping done\n");
  exit();
}
