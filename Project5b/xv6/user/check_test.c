#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#if 0
typedef unsigned int uint;
#define CH_MASK (0xff000000)
#define CH_GET_CHECKSUM(addr) (((addr) & CH_MASK) >> 24)
#define CH_PUT_CHECKSUM(addr, val) \
  ((addr) = (((addr) & (~CH_MASK)) | (((uint)((val) & 0xff)) << 24)))
#define CH_GET_ADDR(addr) ((addr) & (~CH_MASK))
#define CH_PUT_ADDR(addr, val) \
  ((addr) = (((addr) & (CH_MASK)) | ((val) & (~CH_MASK)) ))
  unsigned int test = 0xaaaaaaaa;
  printf(1, "Size of uint = %d\n", sizeof(unsigned int));
  CH_PUT_CHECKSUM(test, 0xee);
  printf(1, "test : %x | checksum : %x | addr = %x \n", test,
    CH_GET_CHECKSUM(test), CH_GET_ADDR(test));
  CH_PUT_ADDR(test, 0xbbbbbb);
  CH_PUT_CHECKSUM(test, 0xab);
  printf(1, "test : %x | checksum : %x | addr = %x \n", test,
    CH_GET_CHECKSUM(test), CH_GET_ADDR(test));
#endif

int main(int argc, char *argv[]) {
  int fd;
  int ret = 0;
  int rd = 1;
  char *filename = "1.txt";
  unsigned char arr[1024] = {'a', 'b', 'c', 'd'};
  unsigned char corrupted_data[1024] = {'~','!','#','$','%','^','&'};
  
  if (argc > 1) {
    if (argv[1][0] == 'r')
      rd = 1;
    else if (argv[1][0] == 'w')
      rd = 0;
    else if (argv[1][0] == 'c') {
      /* corrupt the file */
      rd = -1;
    }
  }
  
  if (rd == 1) {
    int i = 0;
    fd =  open(filename, O_CHECKED | O_RDONLY);
    if (fd < 0) {
      printf(2, "%s: Does not exist!\n", filename);
      exit();
    }
    
    ret = read(fd, arr, 1024);
    printf(1, "read ret = %d\n", ret);
    for(i = 0; i < 1024; ++i) {
      if (arr[i]) {
        printf(1, "%d:%c, ", i, arr[i]);
      }
    }
  } else if (rd == 0) {
    fd =  open(filename, O_CREATE | O_CHECKED | O_RDWR);
    
    ret =  write(fd, arr, 1024);
    printf(1, "write ret = %d\n", ret);
  } else if (rd == -1) {
    /* corrupt the file */
    fd =  open(filename, O_CREATE | O_RDWR);
    ret =  write(fd, corrupted_data, 1024);
    printf(1, "corrupt write ret = %d\n", ret);
  }
  close(fd);
  exit();
}
