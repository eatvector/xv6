#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}


uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  struct proc *p=myproc();


  addr = p->sz;
  uint64 newsz=p->sz+n;

 // printf("sbrk call  n is  %x\n ",n);

  /* if(growproc(n) < 0)
    return -1;*/
  
  // change the size of the process and check to return addr or -1.
  if(n>=0){
      if(newsz>MAXSZ){
        return -1;
      }
       p->sz=p->heapvma.end=newsz;
   //  printf("new size %p  NEWSZ %p\n",newsz,MAXSZ);
  } else {
    // we need to free some memory,but it may not be alloc before.
    // return new  sz or just panic
     printf("decrease the memory   new size:%d",newsz);
     if(newsz>=p->heapvma.begin){
      // free the memory
       uvmdealloc(p->pagetable, p->sz, newsz);
     }else{
       return -1;
     }
  }
 // printf("heap range :%p   to %p\n",)
  p->sz=p->heapvma.end=newsz;
 //printf("heap range :%p   to %p\n",p->heapvma.begin,p->heapvma.end);
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
