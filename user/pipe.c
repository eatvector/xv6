#include "kernel/types.h"
#include "user/user.h"

// pipe2.c: communication between two processes

int
countfree()
{
  int fds[2];

  if(pipe(fds) < 0){
    printf("pipe() failed in countfree()\n");
    exit(1);
  }
  
  int pid = fork();

  if(pid < 0){
    printf("fork failed in countfree()\n");
    exit(1);
  }

  if(pid == 0){
    close(fds[0]);
    
    while(1){
      uint64 a = (uint64) sbrk(4096);
      if(a == 0xffffffffffffffff){
        break;
      }

      // modify the memory to make sure it's really allocated.
      *(char *)(a + 4096 - 1) = 1;

      // report back one more page.
      if(write(fds[1], "x", 1) != 1){
        printf("write() failed in countfree()\n");
        exit(1);
      }
    }

    exit(0);
  }

  close(fds[1]);

  int n = 0;
  while(1){
    char c;
    int cc = read(fds[0], &c, 1);
    if(cc < 0){
      printf("read() failed in countfree()\n");
      exit(1);
    }
    if(cc == 0)
      break;
    n += 1;
  }

  close(fds[0]);
  wait((int*)0);
  
  return n;
}










int
main()
{
  printf("cntfree :%x\n",countfree());

  exit(0);
}