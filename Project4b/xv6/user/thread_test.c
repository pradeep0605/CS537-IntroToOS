#include "types.h"
#include "stat.h"
#include "user.h"

#define NLOOP 12
void thread_code(void *arg)
{
  int N = NLOOP;
  int i = 0;
  printf(1, "thread_code:%s : N = %d &i = %d(%d)\n", (char*)arg, N, i, &i);
  for (i = 0; i < N; ++i) {
    printf(1, "Thread:%s | i = %d | &i= %d\n", (char*)arg, i, &i);
  }
  exit();
}

int main(int argc, char *argv[])
{
  int tid = 0, tid1 = 0;
  
  tid = thread_create(thread_code, (void*)"A");
  tid1 = thread_create(thread_code, (void*)"B");
  /*
  int i = 0;
  for (i = 0; i < NLOOP; ++i) {
    //printf(1, "%s:%d: %d\n", __func__, tid, i);
  }
  */
  printf(1, "Parent Sleeping\n");
  thread_join(tid);
  thread_join(tid1);
  printf(1, "Sleeping done\n");
  exit();
}
