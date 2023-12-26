// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


//this code have some bugs

//Allocate n pages ,each page have 4096-byte.
/*
void *
kallocnpages(uint64 n){
   struct run *r;
   struct run *r0;
   struct run *header;
   uint64 i=0;
   acquire(&kmem.lock);
   r=header=kmem.freelist;
   for(;i<n;i++){
    r0 = kmem.freelist;
    if(r0)
    kmem.freelist= r0->next;
    else{
      r=(struct run *)0;
      kmem.freelist=header;
      break;
    }
   }
   release(&kmem.lock);
   if(r){
     for(uint64 i=0;i<n;i++){
         memset((char*)r+i*PGSIZE, 5, PGSIZE); // fill with junk
     }
   }
   return (void *)r;
}*/


 int kallocnpages(uint64 *addrs,uint npages){
      if(npages>MAXNPAGES){
        return -1;
      }
    for(uint  i=0;i<npages;i++){
      if((addrs[i]=(uint64)kalloc())==0){
          return -1;
      }
    }
    return 0;
}

void kfreenpages(uint64 *addrs,uint npages){
   
   for(uint i=0;i<npages;i++){
     kfree((void *)addrs[i]);
   }
}
