#ifndef SBI_H
#define SBI_H
#include "types.h"

#define INTERVAL 1000000    // about 1/10th second in qemu.

#define CAUSE_TIMER  0x8000000000000007
#define CAUSE_SECALL 0x9    //form S mode
#define CAUSE_SOFTWARE  0x8000000000000003

#define SBI_IPI 0


struct sbi_trap_regs{         
  uint64 ra;
  uint64 sp;
  uint64 gp;
  uint64 tp;
  uint64 t0;
  uint64 t1;
  uint64 t2;
  uint64 s0;
  uint64 s1;
  uint64 a0;
  uint64 a1;
  uint64 a2;
  uint64 a3;
  uint64 a4;
  uint64 a5;
  uint64 a6;
  uint64 a7;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
  uint64 t3;
  uint64 t4;
  uint64 t5;
  uint64 t6;

  uint64 mepc;        
  uint64 mcause;
};



#define SBI_CALL(which,arg0,arg1,arg2)  ({   \
    register uint64 a0 asm ("a0") =(uint64)(arg0); \
    register uint64 a1 asm ("a1") =(uint64)(arg1); \
    register uint64 a2 asm ("a2") =(uint64)(arg2); \
    register uint64 a7 asm ("a7") =(uint64)(which); \
    asm volatile ("ecall"  \
                  : "+r" (a0)  \
                  :"r" (a1),"r"(a2),"r"(a7)  \
                  :"memory" ); \
                  a0; \
})


#define SBI_CALL_0(which) SBI_CALL(which,0,0,0)
#define SBI_CALL_1(which,arg0) SBI_CALL(which,arg0,0,0)
#define SBI_CALL_2(which,arg0,arg1) SBI_CALL(which,arg0,arg1,0)

#endif

