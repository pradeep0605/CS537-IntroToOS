#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  void *addr = (void *)0x9D000;
  int *a;
  int *b;
  addr = shmgetat(1, 1);
  a = (int *) addr;
  *a = 10;
  printf(1, "PID = %d Return address = %x, *a = %d refcount = %d\n", getpid(),
    (int) addr, *a, shm_refcount(1));

  addr = shmgetat(2, 1);
  b = (int *) addr;
  *b = 20;
  printf(1, "PID = %d Return address = %x, *b = %d refcount = %d\n", getpid(),
    (int) addr, *b, shm_refcount(2));
  exit();
}
