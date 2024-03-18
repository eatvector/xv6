
struct spinlock tlblock;

void init_tlb_lock(){
  initlock(&tlblock,"tlblock");
}

extern int cpucnt;

void flush_all_tlb(){
   acquire(&tlblock);
    int id=cpuid();
// need modify here
  for(int i=0;i<cpucnt;i++){
    if(i!=id)
     SBI_CALL(SBI_IPI,i,0,0);
  }
  release(&tlblock);
}

