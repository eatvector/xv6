#include "sbi.h"
#include "types.h"
#include "riscv.h"
#include "memlayout.h"
#include "param.h"
#include "defs.h"

// running in M mode
//
__attribute__ ((aligned (16))) char stack1[4096 * NCPU];

int tlb_sync;//is tlb flush

void timer_handler(struct sbi_trap_regs*regs){
    // do someting
    /*
      # schedule the next timer interrupt
        # by adding interval to mtimecmp.
       # ld a1, 24(a0) # CLINT_MTIMECMP(hart)
        #ld a2, 32(a0) # interval
        #ld a3, 0(a1)
        #add a3, a3, a2
       # sd a3, 0(a1) #clear mip mtip

        # arrange for a supervisor software interrupt
        # after this handler returns.
       # li a1, 2
        #csrw sip, a1 #set mip  

        #ld a3, 16(a0)
        #ld a2, 8(a0)
        #ld a1, 0(a0)
        #csrrw a0, mscratch, a0
        #to usermode or mode
    */
    int hartid=r_mhartid();
    uint64 mtimecmp= *(uint64*)CLINT_MTIMECMP(hartid);
    mtimecmp+=INTERVAL;
    *(uint64*)CLINT_MTIMECMP(hartid)=mtimecmp;
    
    //raise a s mode soft interrupt 
    w_sip(2);
}

void ipi_handler(){
    //todo
     
     int hartid=r_mhartid();
    *(uint32*)CLINT_MSIP(hartid)=0;
    // mip 
     w_mip(r_mip() & ~8);
    sfence_vma();
    __sync_synchronize();
    //  tlb is flush
    // in M mode we do not use any pagetable
    printf("hart %d flush tlb\n",hartid);
    tlb_sync=1;
}


void send_ipi(int hartid){
    //should not send it self.
    // return if the gobjict 
    if(hartid==r_mhartid()){
        //send to its self ,qucik return 
        return ;
    }
    tlb_sync=0;
       __sync_synchronize();
    *(uint32*)CLINT_MSIP(hartid)=1;
    while(tlb_sync==0);
    __sync_synchronize();
    tlb_sync=0;
    return ;
}


void secall_handler(uint64 ecall_id,struct sbi_trap_regs*regs){

    switch(ecall_id){
        case SBI_IPI:
        // is a0?
         send_ipi(regs->a0);
         break;
         default:
         break;
    }
}

void sbi_trap_hangler(struct sbi_trap_regs*regs){
      
    uint64 mcause=r_mcause();
    uint64 ecall_id=regs->a7;

    switch(mcause){
        case CAUSE_TIMER:
        timer_handler(regs);break;
        case CAUSE_SOFTWARE:
          ipi_handler();break;
        case CAUSE_SECALL:
        secall_handler(ecall_id,regs);
        regs->mepc+=4;
                  break;
         default:
         break;
    }
}


