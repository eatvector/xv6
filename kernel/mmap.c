
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "fcntl.h"
#include "memlayout.h"

//return 0 if is mmap,-1 if is load page fault
int mmapfile(uint64 addr){

    struct proc *p=myproc();
    struct vma*vi=0;
    struct vma*v=0;
    
    for(int i=0;i<NVMA;i++){
        if((vi=p->mapregiontable[i])!=0){
            if(addr>=(uint64)(vi->addr)&&addr<(uint64)(vi->addr)+vi->lenth){
                v=vi;
                break;
            }
        }
    }

    if(v==0){
        return -1;
    }

   // we map file from this space
    addr=PGROUNDDOWN(addr);
    //printf("map at addr:%d\n");
   
    char *mem;
    if((mem=kalloc())==0){
        return -1;
    }

     int perm= PTE_U;

    if(v->prot&PROT_READ){
        perm|=PTE_R;
    }
    if(v->prot&PROT_WRITE){
        perm|=PTE_W;
    }

    if(perm==PTE_U){
        panic("perm should not be zero\n");
    }
    
    // we not fuck map this 
    memset(mem,0,PGSIZE);
    if(mappages(p->pagetable,addr,PGSIZE,(uint64)mem,perm)!=0){
        panic("mmap map fail\n");
    }

    ilock(v->f->ip);
    //read file to physical memory
    // fuck mot pgsize

    readi(v->f->ip,1,addr,addr-(uint64)v->addr+v->off,PGSIZE);
    int s=(addr-(uint64)v->addr)/PGSIZE;
    v->inmemory|=(1<<s);
    //change in_memory
    iunlock(v->f->ip);
    return 0;
}


int  munmapfile(uint64 addr,uint len){
    //addr is in  start or end ,len is 0 or the whole file
    // addr must be page aligned
    // we have find the region
    // lenth must be pagealigned

    if(len>MMAPMAXLENTH||len%PGSIZE!=0){
      return -1;
    }

    struct proc *p=myproc();
    struct vma*vi=0;
    struct vma*v=0;

    int i=0;

    for(;i<NVMA;i++){
        if((vi=p->mapregiontable[i])!=0){
            if(addr>=(uint64)(vi->addr)&&addr<(uint64)(vi->addr)+vi->lenth){
                v=vi;
                break;
            }
        }
    }

    if(v==0){
        return -1;
    }


    //check the args
    if(len%PGSIZE||len>v->lenth){
        // lenth error
        return -1;
    }

    //out of range
    if(addr+len>(uint64)v->addr+v->lenth){
        return -1;
    }
    
    // can not handle this situation
    if(addr!=v->addr&&addr+len!=(uint64)v->addr+v->lenth){
        return -1;
    }


    //layzy allocation may not in memory
    uint64 umap_addr=addr;
    int r=0;
    // this is very important
    int s=(addr-v->addr)/PGSIZE;
   

    for(;umap_addr<addr+len;umap_addr+=PGSIZE,s++){
        // check if is in memory
        //int inmemory=(walkaddr(p->pagetable,umap_addr)!=0);
        int inmemory=((v->inmemory&(1<<s))!=0);

        if(v->flags==MAP_SHARED&&inmemory){
            begin_op();
            ilock(v->f->ip);
            if( writei(v->f->ip, 1,umap_addr,umap_addr-v->addr+v->off,PGSIZE)!=PGSIZE){
                r=-1;
            }
            iunlock(v->f->ip);
            end_op();
        }

        if(r==-1){
            return -1;
        }

        if(inmemory){
            uvmunmap(p->pagetable,umap_addr,1,1);
            v->inmemory&=~(1<<s);
        }
    }

    v->lenth-=len;
    if(addr==(uint64)v->addr){
        v->addr+=len;
        v->off+=len;
        v->inmemory=(v->inmemory)>>(len/PGSIZE);
    }

    if(v->lenth==0){
       fileclose(v->f);
       p->mapregiontable[i]=0;
       vmafree(v);
       // free the fuck bitbmap
       int mask=(1<<(addr-MMAPADDR)/MMAPMAXLENTH);
       p->mmapbitmap|=mask;
    }

    return 0;
}


//used in exit
void munmapallfile(){
    
    struct proc *p=myproc();
    struct vma *v=0;
    for(int i=0;i<NVMA;i++){
        if((v=p->mapregiontable[i])!=0){
          munmapfile(v->addr,v->lenth);
        }
    }
}





