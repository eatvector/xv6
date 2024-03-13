#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include"spinlock.h"



#include "sbi.h"

volatile static int started = 0;
int cpucnt=0;
struct spinlock  cpucntlock;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  //in this we can receive M timer interrupt
  //can not receive S mode interrupt until scheduler call instr_on
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode table
    fileinit();      // file table
    vmainit();
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    init_tlb_lock();
    initlock(&cpucntlock,"cpucntlock");
    acquire(&cpucntlock);
    cpucnt++;
    release(&cpucntlock);
    __sync_synchronize();
    started = 1;
     printf("hart %d starting\n",cpuid());
    // flush_all_tlb();
    //to here the interrupt is disabble(S and M)

  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    //hold lock ,a ierrupt arrive.
    // soft interrupt
    printf("hart %d starting\n", cpuid());
    //flush_all_tlb();
    acquire(&cpucntlock);
    cpucnt++;
    release(&cpucntlock);
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

// open interrupt 
  scheduler();        
}
