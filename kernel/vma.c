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
    vma->end=0;
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
    dst->end=src->end;
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

    //vm
    enter_vm();

    int ret=0;
    begin_op();

for(;va<end;va+=PGSIZE){

  if(va>=MAXVA){
     ret= -1;
  }else{
    pte_t *pte=walk(p->pagetable,va,0);
    // user code can not access this page.
    if(pte&&*pte&PTE_V){
      //pte is valid,but usercode can not access this page.
          if((*pte&PTE_U)==0){
             ret=-1;
          }else if((*pte&PTE_COW)&&cow){
            ret=uvmcow(p->pagetable,va);
         }else{
           // do nothing handler
           // may stll have some bug here.
           // 
           //juge load and store?
           uint scause=r_scause();
           uint  pagelost=(scause==12||scause==13);
           //scause=
           if(pagelost){
              //the page has been load by other thread
              ret=0;
           }else{
             // write to read only page.
             ret=-1;
           }


         }
    }else{
        
        //lost some pages.
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
  end_op();
  // flush the tlb.
  leave_vm(1);
  return ret;
}

// only one thread will change the vm space,other thread will sleep.
void enter_vm(){
   struct proc *p=myproc();
   acquiresleep(&p->vmalook);
}

void leave_vm(int doflush){
    struct proc *p=myproc();
    if(doflush){
     flush_all_tlb();
   }
   releasesleep(&p->vmalook);
   /*if(doflush){
    flush_all_tlb();
   }*/
   // after do this ,we can ensure that ,all other core has flush it's tlb;
}