#include "types.h"
#include "spinlock.h"
#include "vma.h"
#include "param.h"
#include "defs.h"
#include"proc.h"

struct {
  struct spinlock lock;
  struct vma vma[NVMA];
}vmatable;


void
vmainit(void)
{
  initlock(&vmatable.lock,"vmatable");
}

struct vma*vmaalloc(void){
  acquire(&vmatable.lock);
  for(int i=0;i<NVMA;i++){
     if(vmatable.vma[i].isalloc==0){
        vmatable.vma[i].isalloc=1;
        release(&vmatable.lock);
        return &vmatable.vma[i];
     }
  }
  release(&vmatable.lock);
  return 0;
}

void vmafree(struct vma*vma){
    acquire(&vmatable.lock);
    vma->addr=0;
    vma->f=0;
    vma->off=0;
    vma->isalloc=0;
    vma->lenth=0;
    vma->flags=0;
    vma->prot=0;
    vma->inmemory=0;
    vma->filesz=0;
    vma->ip=0;
    release(&vmatable.lock);
}

void vmacopy(struct vma*src,struct vma *dst){
    dst->addr=src->addr;
    // Don't forget to increment the reference count for a VMA's struct file. 
    // i refs(how many pointer point to it)
    if(src->f)
        dst->f=filedup(src->f);
    else 
       dst->f=0;
    dst->flags=src->flags;
    dst->lenth=src->lenth;
    dst->prot=src->prot;
    dst->off=src->off;
    dst->memsz=src->memsz;
    dst->filesz=src->filesz;
    if(src->ip){
      dst->ip=idup(src->ip);
    }
    dst->inmemory=src->inmemory;
}

//load a page 
// for 
// should exit
static int lazyallocate(uint64 va)
{
      char *mem=0;
      struct proc*p=myproc();
      //heap vma
      struct vma*vma=p->vma[NPMMAPVMA];
      if(va>=vma->addr&&va< vma->end){
        //already mapped
     
         // load the page
       //  printf("heap vma start :%p  end :%p in heap\n",vma->begin,vma->end);
         uint64 a=PGROUNDDOWN(va);
         mem=kalloc();
         if(mem==0){
           return -1;
         }
         memset(mem, 0, PGSIZE);
         if(mappages(p->pagetable, a, PGSIZE, (uint64)mem, PTE_R|PTE_U|PTE_W) != 0){
            kfree(mem);
           return -1;
          }
    }else{
      return 1;
    }
    return 0;
}


//return 0 or -1.
int pagefaulthandler(uint64 va,uint64 n,int cow){
    uint64 end=va+n;
    va=PGROUNDDOWN(va);
    struct proc *p=myproc();
    
    int ret=0;
   

for(;va<end;va+=PGSIZE){

  if(va>=MAXVA){
     ret= -1;
  }else{
    pte_t *pte=walk(p->pagetable,va,0);
    if(pte&&*pte&PTE_V){
         if(*pte&PTE_COW&&cow){
            ret=uvmcow(p->pagetable,va);
         }else{
           // do nothing handler
           ret=1;
         }
    }else{
        ret=mmap(va);
        if(ret==1){
          ret=lazyallocate(va);
          //set ret 0 or -1
          if(ret==1)
           ret=exechandler(va);
        }
    }
  }

    if(ret==-1){
      break;
    }
}
    return ret;
}

