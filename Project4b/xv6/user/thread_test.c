#include "types.h"
#include "stat.h"
#include "user.h"

#define NLOOP 12
lock_t l;

void nested_thread(void *arg) {
  int i  = 0;
  for (i = 0; i < NLOOP; ++i) {
    printf(1, "Thread:%s | i = %d | &i= %d\n", (char*)arg, i, &i);
  }
  exit();
}

void thread_code(void *arg)
{
  int N = NLOOP;
  int i = 0;
  int tid = thread_create(nested_thread, "Nested");
  printf(1, "Waiting for child thread %d\n", tid);
  thread_join();
  printf(1, "child thread %d dead!\n", tid);

  for (i = 0; i < N; ++i) {
    lock_acquire(&l);
    printf(1, "Thread:%s | i = %d | &i= %d\n", (char*)arg, i, &i);
    lock_release(&l);
  }
  exit();
}

int main(int argc, char *argv[])
{
  int tid = 0, tid1 = 0, temp;
  lock_init(&l);
  tid = thread_create(thread_code, (void*)"A");
  tid1 = thread_create(thread_code, (void*)"B");
  tid1 = tid;
  tid = tid1;
  /*
  int i = 0;
  for (i = 0; i < NLOOP; ++i) {
    //printf(1, "%s:%d: %d\n", __func__, tid, i);
  }
  */
  printf(1, "Parent waiting for threads to join\n");
  temp = thread_join();
  printf(1, "Child thread %s is dead!\n", (temp == tid) ? "A" :  "B");
  temp = thread_join();
  printf(1, "Child thread %s is dead!\n", (temp == tid) ? "A" :  "B");
  printf(1, "All threads joined!\n");
  exit();
}
