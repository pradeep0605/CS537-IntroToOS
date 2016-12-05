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
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
char output[1000] = {0};
#define BLOCK_SIZE (512)

int nblocks = 995;
int ninodes = 200;
int size = 1024;
char my_bitmap[512];
//char my_inode[200];
char my_inode_alloced[200];
char my_dir_parent_inode[200];
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

void check_inode_alloced (uint inode_num, void* img_ptr) {
  struct dinode *dip = (struct dinode*)rsect(2, img_ptr);
  if (0 == dip[inode_num].type) {
    fscheck_perror("ERROR: inode referred to in directory but marked free.\n");
    exit(1);
  }
  //if (T_FILE == dip[inode_num].type) {
  //  my_inode[inode_num]--;
  //  //printf("%d found. Left %d\n", inode_num, my_inode[inode_num]);
  //}
}

void check_balloced(uint block, void* img_ptr) {
  char* bitmap = rsect(BBLOCK(block, ninodes), img_ptr);
  uint off = block / 8;
  uint bit_off = block % 8;
  if (0x01 != ((bitmap[off] >> (bit_off)) & 0x01)) {
    fscheck_perror("ERROR: address used by inode but marked free in bitmap.\n");
    exit(1);
  }
  if (0x0 == ((my_bitmap[off] >> (bit_off)) & 0x01)) {
    fscheck_perror("ERROR: address used more than once.\n");
    exit(1);
  }
  my_bitmap[off] &= (~(0x1 << bit_off));
  //printf ("bl:%d, bp:"BYTE_TO_BINARY_PATTERN"\n", block, BYTE_TO_BINARY(bitmap[off]));
  return;
}

void check_parent_dir(uint parent_inode, uint child_inode, void* img_ptr) {
  struct dinode* inode_start = (struct dinode*)rsect(2, img_ptr);
  struct dinode* dip = &(inode_start[parent_inode]);
  uint seen_child = 0;
  if (dip->type == T_DIR) {
    uint file_size = dip->size;
    uint n_blocks = file_size / 512;
    int i = 0;
    char *buf;
    uint* indirect = NULL;
    if (n_blocks > NDIRECT) indirect = rsect(dip->addrs[NDIRECT], img_ptr);
    for (i=0; i< n_blocks; i++) {
      if (i < NDIRECT) {
        buf = rsect(dip->addrs[i], img_ptr);
        struct xv6_dirent *dir_entry = (struct xv6_dirent*)buf;
        while (0 != dir_entry->inum) {
          if (dir_entry->inum == child_inode) {
            seen_child = 1;
            break;
          }
          dir_entry++;
        }
      }
      else {
        uint curr_addr = indirect[i-NDIRECT];
        buf = rsect(curr_addr, img_ptr);
        struct xv6_dirent *dir_entry = (struct xv6_dirent*)buf;
        while (0 != dir_entry->inum) {
          if (dir_entry->inum == child_inode) {
            seen_child = 1;
            break;
          }
          dir_entry++;
        }
      }
    }
  }
  if (0 == seen_child) {
    fscheck_perror("ERROR: parent directory mismatch.\n");
    exit(1);
  }
}

int seen_root = 0;
void check_dir_inode (uint inode_num, struct dinode* dip, void* img_ptr) {
  struct dinode* inode_start = (struct dinode*)rsect(2, img_ptr);
  uint file_size = dip->size;
  uint n_blocks = file_size / 512;
  if (file_size % 512) n_blocks++;
  int i = 0;
  char *buf;
  uint* indirect = NULL;
  if (n_blocks > NDIRECT) indirect = rsect(dip->addrs[NDIRECT], img_ptr);
  int seen_self_dir = 0, seen_parent_dir = 0;
  for (i=0; i< n_blocks; i++) {
    if (i < NDIRECT) {
      buf = rsect(dip->addrs[i], img_ptr);
      struct xv6_dirent *dir_entry = (struct xv6_dirent*)buf;
      while (0 != dir_entry->inum) {
        if (0 == seen_self_dir && (0 == strcmp(dir_entry->name, "."))) seen_self_dir = 1;
        else if (0 == seen_parent_dir && (0 == strcmp(dir_entry->name, ".."))) {
          seen_parent_dir = 1;
          if (0 == seen_root && dir_entry->inum == inode_num && inode_num == 1) seen_root = 1;
          check_parent_dir(dir_entry->inum, inode_num, img_ptr);
        } else {
          if (inode_start[dir_entry->inum].type == T_DIR) {
            if (my_dir_parent_inode[dir_entry->inum] == 0) {
              my_dir_parent_inode[dir_entry->inum] = inode_num;
            } else {
              fscheck_perror("ERROR: directory appears more than once in file system.\n");
              exit(1);
            }
          }
        }
        check_inode_alloced(dir_entry->inum, img_ptr);
        my_inode_alloced[dir_entry->inum] = 0;
        //printf ("%d,%s,%d\n",inode_num, dir_entry->name, dir_entry->inum);
        dir_entry++;
      }
    }
    else {
      uint curr_addr = indirect[i-NDIRECT];
      buf = rsect(curr_addr, img_ptr);
      struct xv6_dirent *dir_entry = (struct xv6_dirent*)buf;
      while (0 != dir_entry->inum) {
        if (0 == seen_self_dir && (0 == strcmp(dir_entry->name, "."))) seen_self_dir = 1;
        else if (0 == seen_parent_dir && (0 == strcmp(dir_entry->name, ".."))) {
          seen_parent_dir = 1;
          if (0 == seen_root && dir_entry->inum == inode_num && inode_num == 1) seen_root = 1;
          check_parent_dir(dir_entry->inum, inode_num, img_ptr);
        } else {
          if (inode_start[dir_entry->inum].type == T_DIR) {
            if (my_dir_parent_inode[dir_entry->inum] == 0) {
              my_dir_parent_inode[dir_entry->inum] = inode_num;
            } else {
              fscheck_perror("ERROR: directory appears more than once in file system.\n");
              exit(1);
            }
          }
        }
        check_inode_alloced(dir_entry->inum, img_ptr);
        my_inode_alloced[dir_entry->inum] = 0;
        //printf ("%d,%s,%d\n",inode_num, dir_entry->name, dir_entry->inum);
        dir_entry++;
      }
    }
  }
  if (!(seen_self_dir & seen_parent_dir)) {
    fscheck_perror("ERROR: directory not properly formatted.\n");
    exit(1);
  }
}

void check_inode_addrs(struct dinode* dip, void* img_ptr) {
  uint file_size = dip->size;
  uint n_blocks = file_size / 512;
  if (file_size % 512) n_blocks++;
  int i;
  uint* indirect = NULL;
  if (n_blocks > NDIRECT) {
    uint curr_addr = dip->addrs[NDIRECT];
    //printf ("D->I Addr:%d\n", curr_addr);
    if (curr_addr < 29 || curr_addr > 1024) {
      fscheck_perror("ERROR: bad address in inode.\n");
      exit(1);
    }
    check_balloced(curr_addr, img_ptr);
    indirect = rsect(curr_addr, img_ptr);
  }
  for (i=0; i< n_blocks; i++) {
    if (i < NDIRECT) {
      uint curr_addr = dip->addrs[i];
      //printf ("D Addr:%d\n", curr_addr);
      if (curr_addr < 29 || curr_addr > 1024) {
        fscheck_perror("ERROR: bad address in inode.\n");
        exit(1);
      }
      check_balloced(curr_addr, img_ptr);
    }
    else {
      uint curr_addr = indirect[i-NDIRECT];
      //printf ("I Addr%d\n", curr_addr); 
      if (curr_addr < 29 || curr_addr > 1024) {
        fscheck_perror("ERROR: bad address in inode.\n");
        exit(1);
      }
      check_balloced(curr_addr, img_ptr);
    }
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
  //fscheck_perror("ERROR: bad inode.\n");
  //exit(1);
  char* bitmap = rsect(28, img_ptr);
  int i;
  for (i=0; i < 512; i++) {
      my_bitmap[i] = bitmap[i];
      //printf ("%d,mybp:"BYTE_TO_BINARY_PATTERN", bp:"BYTE_TO_BINARY_PATTERN"\n", i, BYTE_TO_BINARY(my_bitmap[i]), BYTE_TO_BINARY(bitmap[i]));
  }
  struct dinode *dip = (struct dinode*)rsect(2, img_ptr);
  for (i = 0; i< sb->ninodes; i++ ) {
    if (dip[i].type) my_inode_alloced[i] = 1;
    //if (dip[i].type == T_FILE) {
    //  my_inode[i] = dip[i].nlink;
    //  //printf("%d:%d referred %d times\n", dip[i].type, i, dip[i].nlink);
    //}
  }
  for (i = 0; i< sb->ninodes; i++) {
    switch(dip->type) {
      case 0:
        break;
      case T_DEV:
      case T_FILE: {
        //printf ("File addresses\n");
        check_inode_addrs(dip, img_ptr);
        //printf ("File addresses end\n");
        break;
      }
      case T_DIR: {
        //printf ("Dir addresses\n");
        check_inode_addrs(dip, img_ptr);
        //printf ("Dir addresses end\n");
        check_dir_inode (i, dip, img_ptr);
        break;
      }
      default:
      fscheck_perror("ERROR: bad inode.\n");
      exit(1);
    }
    dip++;
  }
  if (0 == seen_root) {
    fscheck_perror("ERROR: root directory does not exist.\n");
    exit(1);
  }
  for (i=0; i< 29; i++) {
    check_balloced(i, img_ptr);
  }
  for (i=0; i < 512; i++) {
    //printf ("%d,mybp:"BYTE_TO_BINARY_PATTERN", bp:"BYTE_TO_BINARY_PATTERN"\n", i,BYTE_TO_BINARY(my_bitmap[i]), BYTE_TO_BINARY(bitmap[i]));
    if (my_bitmap[i] != 0) {
      //printf ("%d,mybp:"BYTE_TO_BINARY_PATTERN", bp:"BYTE_TO_BINARY_PATTERN"\n", i,BYTE_TO_BINARY(my_bitmap[i]), BYTE_TO_BINARY(bitmap[i]));
      fscheck_perror("ERROR: bitmap marks block in use but it is not in use.\n");
      exit(1);
    }
  }
  for (i= 0; i< 200; i++) {
    if (1 == my_inode_alloced[i]) {
      fscheck_perror("%d:ERROR: inode marked use but not found in a directory.\n", i);
      exit(1);
    }
  }
  //for (i= 0; i< 200; i++) {
  //  if (0 < my_inode[i]) {
  //    fscheck_perror("%d:ERROR: bad reference count for file.\n",i);
  //    exit(1);
  //  }
  //}
  return 0;
}
