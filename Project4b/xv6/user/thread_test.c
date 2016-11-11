#include "types.h"
#include "stat.h"
#include "user.h"

#define NLOOP 10
#define SLEEP 10
void thread_code(void *arg)
{
  int N = (int) arg;
  int i = 0;
  for (i = 0; i < N; ++i) {
    printf(1, "%s: %d\n", __func__, i);
    sleep(SLEEP);
  }
  exit();
}

int main(int argc, char *argv[])
{
  int i = 0;
  thread_create(thread_code, (void*) NLOOP);

  for (i = 0; i < NLOOP; ++i) {
    printf(1, "%s: %d\n", __func__, i);
    sleep(SLEEP);
  }

  exit();
}
