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


// to trace each puhysical page's references(the physical page TRACE must be set)

//#define TRACE  0x10000
//#define REF_MASK  0xFFFF

static uint phypage_refs[PHYSTOP/PGSIZE];


void  increase_ref(uint64 pa){
  if((pa % PGSIZE) != 0 ||pa >= PHYSTOP){
    panic("increase_ref");
  }
  uint64 i=pa/PGSIZE;
    if(pa>=PGROUNDUP((uint64)end) ){
      phypage_refs[i]++;
    } 
}

void decrease_ref(uint64 pa){
    if((pa % PGSIZE) != 0 ||pa >= PHYSTOP){
      panic("decrease_ref");
    }
    uint64 i=pa/PGSIZE;
    if(pa>=PGROUNDUP((uint64)end)){
      phypage_refs[i]--;
    }
}



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
  memset(phypage_refs,0,sizeof(phypage_refs));
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

  if(phypage_refs[(uint64)pa/PGSIZE]){
   // printf("Can not free refs:%d\n",page_ref&REF_MASK);
    return ;
  }

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

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    phypage_refs[(uint64)r/PGSIZE]=0;
  }

  return (void*)r;
}


