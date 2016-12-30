#include "types.h"
#include "stat.h"
#include "user.h"

#define stdout 1
#define stderr 2

void
print_usage() {
    printf(stderr, "Usage: syscallptest N\n");
}


int main(int argc, char *argv[]) {
  int n, i = 0;
  int before = 0, after = 0;
  before = getnumsyscallp();
  if (argc != 2) {
    print_usage();
    goto end;
  }
  n = atoi(argv[1]);
  
  for (i = 0; i < n; ++i) {
    getpid();
  }
  
  after = getnumsyscallp();
  printf(stdout, "%d\n%d\n", before, after);
end:
  exit();
}

