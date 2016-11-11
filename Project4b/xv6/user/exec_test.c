#include "types.h"
#include "stat.h" 
#include "user.h" 

char *echoargv[] = { "echo", "ALL", "TESTS", "PASSED", 0 };

int main (int argc, char *argv[])
{

  if (fork() == 0) {
    printf(1, "Child\n");
    exec("thread_test", echoargv);
    printf(1, "ExecFailed\n");
    exit();
  } else {
    wait();
    printf(1, "parent\n");
  }
  exit();
}
