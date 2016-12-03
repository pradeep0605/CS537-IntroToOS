#include "include/types.h"
#include "include/fs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char output[1000] = {0};
#define BLOCK_SIZE (512)

int nblocks = 995;
int ninodes = 200;
int size = 1024;
#define fscheck_printf(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDOUT_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDOUT\n");

#define fscheck_perror(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDERR_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDERR\n");

// @brief Function to print out usage of this program in case of invalid args
void usage_and_exit() {
  fscheck_printf("Usage: fscheck file_system_image\n");
  exit(1);
}

int
main(int argc, char* argv[]) {
  if (argc > 2) usage_and_exit();
  char* img_file = argv[1];
  int fd = open(img_file, O_RDONLY);
  if (fd < 0) {
    fscheck_perror("image not found.\n");
    exit(1);
  }

  int rc;
  struct stat sbuf;
  rc = fstat(fd, &sbuf);
  assert(rc == 0);

  void* img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(img_ptr != MAP_FAILED);

  struct superblock* sb = (struct superblock*)(img_ptr + BSIZE);

  int i;
  struct dinode *dip = (struct dinode*) (img_ptr + (2*BSIZE));
  for (i = 0; i< sb->ninodes; i++) {
    switch(dip->type) {
      case 0:
      case 1:
      case 2:
      case 3:
        break;
      default:
      fscheck_perror("bad inode.\n");
    }
    dip++;
  }
  return 0;
}
