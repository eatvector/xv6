#include "types.h"
#include "spinlock.h"
#include "vma.h"
#include "param.h"
#include "defs.h"

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