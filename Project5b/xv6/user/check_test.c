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

int main(int argc, char *arg[]) {
  int fd = open("test.txt", O_CREATE | O_CHECKED | O_RDWR);
  int ret = 0;
  printf(1, "%d\n", fd);
  unsigned char arr[] = {'a', 'b', 'c', 'd'};
  ret =  write(fd, arr, 4);
  printf(1, "ret = %d\n", ret);
  close(fd);
  exit();
}
