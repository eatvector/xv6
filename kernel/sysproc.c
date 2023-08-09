#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
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
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
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

int flag=0;
//#define LAB_PGTBL
#ifdef LAB_PGTBL
#define MAX_PAGES_SCAN PGSIZE
// Reports which pages have been accessed
int 
sys_pgaccess(void ){
   flag=1;
   uint64 ubase;
   int pglen;
   uint64 umask;
   argaddr(0,&ubase);
   argint(1,&pglen);
   argaddr(2,&umask);

   if(pglen>MAX_PAGES_SCAN){
     //starting virtual address should be aligned
     //pages to sacn is too much
     //printf("ubase:0x%p len:%d  umask:0x%p\n",ubase,pglen,umask);
     //panic("out of range");
     return -1;
   }

   char *kmask=(char *)kalloc();
   memset(kmask,0,PGSIZE);
   uint64 va=PGROUNDDOWN(ubase);
   //printf("start page:0x%p\n",va);
   //printf("ubase:0x%p len:%d  umask:0x%p\n",ubase,pglen,umask);
   //uint64 va_end=
   pagetable_t pagetable=myproc()->pagetable;
   
   for(int i=0;i<pglen;i++){
     
     pte_t* pte=walk(pagetable,va,0);

     if(pte==(pte_t*)0){
       //return -1
       //panic("pte is null");
       return -1;
     }
     if((*pte)&PTE_A){
       // add to mask
        char *p=&kmask[i/8];
        *p|=(1<<(i%8));
       //clean PTE_A
       (*pte)&=(~PTE_A);
     }
     va+=PGSIZE;
   }
   
   int nbytes=(pglen/8)+((pglen%8)?1:0);
  
   if(copyout(pagetable,umask,kmask,nbytes)<0){
     //panic("copy out");
     return -1;
   }
   //vmprint(pagetable);
   kfree(kmask);
   return 0;
}

#endif

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


