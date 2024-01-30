#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fs.h"
#include "user/user.h"

void mmap_test();
char buf[BSIZE];
//char superbuf[2*BSIZE];

#define MAP_FAILED ((char *) -1)

int
main(int argc, char *argv[])
{
  mmap_test();
 
  printf("mmaptest: all tests succeeded\n");
  exit(0);
}

char *testname = "???";

void
err(char *why)
{
  printf("mmaptest: %s failed: %s, pid=%d\n", testname, why, getpid());
  exit(1);
}

//
// check the content of the two mapped pages.
//
void
_v1(char *p)
{
  int i;
  for (i = 0; i < PGSIZE*2; i++) {
    if (i < PGSIZE + (PGSIZE/2)) {
      if (p[i] != 'A') {
        printf("mismatch at va:%p\n",p+i);
        printf("mismatch at %d, wanted 'A', got 0x%x\n", i, p[i]);
        err("v1 mismatch (1)");
      }
    } else {
      if (p[i] != 0) {
        printf("mismatch at %d, wanted zero, got 0x%x\n", i, p[i]);
        err("v1 mismatch (2)");
      }
    }
  }
}



//
// create a file to be mapped, containing
// 1.5 pages of 'A' and half a page of zeros.
//
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

  memset(buf+2048,0,BSIZE/2);
  if (write(fd, buf, BSIZE) != BSIZE)
      err("write 0 makefile");




  if (close(fd) == -1)
    err("close");
}



void
mmap_test(void)
{
  int fd;
 
  const char * const f = "mmap.dur";
  printf("mmap_test starting\n");
  testname = "mmap_test";

  //
  // create a file with known content, map it into memory, check that
  // the mapped memory has the same bytes as originally written to the
  // file.
  //
  makefile(f);
  char * p;
    
  printf("test start\n");
  
  // check that mmap does allow read/write mapping of a
  // file opened read/write.
  if ((fd = open(f, O_RDWR)) == -1)
    err("open");
  p = mmap(0, PGSIZE*3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    err("mmap (3)");
  if (close(fd) == -1)
    err("close");

  // check that the mapping still works after close(fd).
  printf("shared v1");
  _v1(p);

  int ret=fork();

  if(ret<0){
    err("fork fail\n");
  }else if(ret==0){
    _v1(p);
    memset(p,'B',PGSIZE);
    munmap(p,PGSIZE*3);
    exit(0);
  }else{
    wait(0);
  }

  printf("start 111\n");
  for(int i=0;i<PGSIZE;i++){
    if(p[i]!='B'){
      printf("got %d\n",p[i]);
      err("should get b  ");
      
    }
  }

   memset(p,'A',PGSIZE);

  munmap(p,PGSIZE*3);

  open(f,O_RDONLY);


read(fd,buf,PGSIZE);
printf("start 222\n");
for(int i=0;i<PGSIZE;i++){
    if(buf[i]!='A'){
      printf("got %d\n",buf[i]);
      printf("got middle %d\n",buf[PGSIZE/2-1]),
      printf("got tail %d\n",buf[PGSIZE-1]);
      err("should get b");
    }
}




}


