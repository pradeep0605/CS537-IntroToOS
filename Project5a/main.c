#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define stat xv6_stat  // avoid clash with host struct stat
#define dirent xv6_dirent  // avoid clash with host struct stat
#include "include/types.h"
#include "include/fs.h"
#include "include/stat.h"
#undef stat
#undef dirent

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

void* rsect (uint sect, void* img_ptr) {
  return img_ptr + (sect*BSIZE);
}

int seen_root = 0;
void check_dir_inode (uint inode_num, struct dinode* dip, void* img_ptr) {
  uint file_size = dip->size;
  uint n_blocks = file_size / 512;
  int i;
  char *buf;
  int seen_self_dir = 0, seen_parent_dir = 0;
  if (n_blocks < NDIRECT) {
    for (i=0; i< n_blocks; i++) {
      buf = rsect(dip->addrs[i], img_ptr);
      struct xv6_dirent *dir_entry = (struct xv6_dirent*)buf;
      while (0 != dir_entry->inum) {
        if (0 == seen_self_dir && (0 == strcmp(dir_entry->name, "."))) seen_self_dir = 1;
        if (0 == seen_parent_dir && (0 == strcmp(dir_entry->name, ".."))) {
          seen_parent_dir = 1;
          if (0 == seen_root && dir_entry->inum == inode_num && inode_num == 1) seen_root = 1;
        }
//        printf ("%d,%s,%d\n",inode_num, dir_entry->name, dir_entry->inum);
        dir_entry++;
      }
    }
  }
  if (!(seen_self_dir & seen_parent_dir)) {
    fscheck_perror("directory not properly formatted.\n");
    exit(1);
  }
}

// unused | superblock | inode table | bitmap (data) | data blocks

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

  struct superblock* sb = (struct superblock*)rsect(1, img_ptr);

  int i;
  struct dinode *dip = (struct dinode*)rsect(2, img_ptr);
  for (i = 0; i< sb->ninodes; i++) {
    switch(dip->type) {
      case 0:
      case T_FILE:
      case T_DEV:
        break;
      case T_DIR: {
        check_dir_inode (i, dip, img_ptr);
        break;
      }
      default:
      fscheck_perror("bad inode.\n");
    }
    dip++;
  }
  if (0 == seen_root) {
    fscheck_perror("ERROR: root directory does not exist.\n");
    exit(1);
  }
  return 0;
}
