#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};



//allocate file struct for f0,f1;
//allocate physical memory for pipe.
int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
  acquire(&pi->lock);
 // pipeacquirecnt++;
  if(writable){
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    //pipereleasecnt++;
    kfree((char*)pi);
  } else{
    release(&pi->lock);
    //pipereleasecnt++;
  }
}

int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();

  //the page may mot load

  /*if(pagefaulthandler(addr,n,0)==-1){
    return -1;
  }*/

  acquire(&pi->lock);
  //pipeacquirecnt++;

  while(i < n){
    if(pi->readopen == 0 || killed(pr)){
      release(&pi->lock);
      // pipereleasecnt++;
      return -1;
    }
    if(pi->nwrite == pi->nread + PIPESIZE){ //DOC: pipewrite-full
      wakeup(&pi->nread);
      // pipereleasecnt++;
      sleep(&pi->nwrite, &pi->lock);
    //  pipeacquirecnt++;
    } else {
      char ch;
      // here we hold (pi->lock) but copyin may cause sched and then the system panic.
     // release(&pi->lock);
      if(copyin(pr->pagetable, &ch, addr + i, 1) == -1){
       // acquire(&pi->lock);
        break;
      }
     // acquire(&pi->lock);

      pi->data[pi->nwrite++ % PIPESIZE] = ch;
      i++;
    }
  }
  wakeup(&pi->nread);
  release(&pi->lock);
  //pipereleasecnt++;
  return i;
}

int
piperead(struct pipe *pi, uint64 addr, int n)
{
  int i;
  struct proc *pr = myproc();
  char ch;

/*  if(pagefaulthandler(addr,n,1)==-1){
    return -1;
  }*/

  acquire(&pi->lock);
 // pipeacquirecnt++;
  while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
    if(killed(pr)){
      release(&pi->lock);
      //pipereleasecnt++;
      return -1;
    }
   // pipereleasecnt++;
    sleep(&pi->nread, &pi->lock); //DOC: piperead-sleep
    //pipeacquirecnt++;
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
    if(pi->nread == pi->nwrite)
      break;
    ch = pi->data[pi->nread++ % PIPESIZE];

   // release(&pi->lock);
    if(copyout(pr->pagetable, addr + i, &ch, 1) == -1){
     // acquire(&pi->lock);
      break;
    }
   // acquire(&pi->lock);
    
  }
  wakeup(&pi->nwrite);  //DOC: piperead-wakeup
  release(&pi->lock);
  //pipereleasecnt++;
  return i;
}
