#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "file.h"
#include "defs.h"


//return 0 if is mmap,-1 if is load page fault
int mmap(uint64 addr){
    
    struct proc *p=myproc();
    struct vma*v=0;
    for(int i=0;i<NVMA;i++){
        if(v=p->mapregiontable[i]){
            // in fila map rregion
            if(addr<p->sz&&addr>=v->addr){
                break;
            }
        }
    }

    if(v==0){
        return -1;
    }

   // we map file from this space
    addr=PGROUNDDOWN(addr);

    mappages(p->pagetable,)

    



}





