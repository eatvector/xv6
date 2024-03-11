#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"



#include "sbi.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
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
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    printf("hart %d starting\n",cpuid());
    __sync_synchronize();
    started = 1;
    //SBI_CALL(SBI_IPI,1,0,0);
    //to here the interrupt is disabble(S and M)

  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    //hold lock ,a ierrupt arrive.
    // soft interrupt
    printf("hart %d starting\n", cpuid());

    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

// open interrupt 
  scheduler();        
}
