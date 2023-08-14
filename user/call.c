#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

int f(int x) {
  return g(x);
}

void main(void) {
  printf("%d %d\n", f(8)+1, 13);
  //unsigned int i = 0x00646c72;
	//printf("H%x Wo%s", 57616, &i);
  int j=0;
  for(int i=0;i<100;i++){
     j++;
  }
  uint64 y=4;
  asm volatile("li a2, %0" : : "i" (y));
  uint64 x;
  asm volatile("mv %0, a2" : "=r" (x) );
  printf("a2:%d\n",x);
  printf("x=%d y=%d\n", 3);
  asm volatile("mv %0, a2" : "=r" (x) );
  printf("a2:%d\n",x);
  exit(0);
}
