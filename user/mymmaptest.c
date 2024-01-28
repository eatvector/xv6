#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fs.h"
#include "user/user.h"

char buf[BSIZE];

void
err(char *why)
{
  
  exit(1);
}

void
makefile(const char *f)
{
  int i;
  int n = PGSIZE/BSIZE;

  unlink(f);
  int fd = open(f, O_WRONLY | O_CREATE);
  if (fd == -1)
    err("open");
  memset(buf, 'A', BSIZE);
  // write 1.5 page
  for (i = 0; i < n + n/2; i++) {
    if (write(fd, buf, BSIZE) != BSIZE)
      err("write 0 makefile");
  }
  if (close(fd) == -1)
    err("close");
}

int 
main(void)
{
  int fd;
  
  const char * const f = "mmap.dur";
  printf("mmap_test starting\n");
  
  makefile(f);
  if ((fd = open(f, O_WRONLY)) == -1)
    err("open");

  printf("test mmap f\n");
 
  char *p = mmap(0, PGSIZE*2, PROT_WRITE|PROT_READ, MAP_PRIVATE, fd, 0);
  *p=1;
  printf("successful write\n");
  printf("test data :%d\n",*p);

  int ret=fork();
  if(ret<0){
      printf("fork fail\n");
  }else if(ret==0){
      printf("kid");
      *p=0;
      printf("test data :%d\n",*p);
      exit(0);
  }else{
    wait(0);
  }
  


return 0;
}




