#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"



int main(int argc, char *argv[]) {
  struct stat s;
  if (argc < 2) {
    printf(2, "Usage: ./fliestat filename\n");
    exit();
  }
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf(2, "%s: Error opening file!\n", argv[1]);
    exit();
  }
  if (fstat(fd, &s) < 0) {
    printf(2, "%s: Error fstat-ing file!\n", argv[1]);
    exit();
  }
  printf(1, "type = %d, dev = %d, ino = %d, nlink = %d, size = %d,"
  " checksum = %x\n", s.type, s.dev, s.ino, s.nlink, s.size, s.checksum);
  exit();
}
