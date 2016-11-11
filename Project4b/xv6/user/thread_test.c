#include "types.h"
#include "stat.h"
#include "user.h"

#define NLOOP 12
#define SLEEP 10
void thread_code(void *arg)
{
  int N = (int) arg;
  int i = 0;
  printf(1, "INSIDE THREAD ! YAAAYY !! n = %d \n\n", N);
  for (i = 0; i < 10; ++i) {
    printf(1, "%s: %d\n", __func__, i);
  }
  exit();
}

int main(int argc, char *argv[])
{
  int i = 0;
  int tid = 0;
  
  tid = thread_create(thread_code, (void*) NLOOP);

  tid = tid;
  for (i = 0; i < NLOOP; ++i) {
    printf(1, "%s:%d: %d\n", __func__, tid, i);
  }
  sleep (1000);
  exit();
}
