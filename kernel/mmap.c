
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
#include "buf.h"

//return 0 if is mmap,-1 if is load page fault
int mmap(uint64 addr){

    struct proc *p=myproc();
    struct vma*vi=0;
    struct vma*v=0;
    
       for(int i=0;i<NPMMAPVMA;i++){
        if((vi=p->mapregiontable[i])!=0){
            if(addr>=(uint64)(vi->addr)&&addr<(uint64)(vi->addr)+vi->lenth){
                v=vi;
                break;
            }
        }
    }

    if(v==0){
        // not in mmap region
        return 1;
    }

    if(addr>=PGROUNDUP(v->f->ip->size)+v->addr){
        //printf("fck size %d addr %d\n",PGROUNDUP(v->f->ip->size));
        return -1;
    }

   // we map file from this space
    addr=PGROUNDDOWN(addr);
    //printf("map at addr:%d\n");
    int s=(addr-(uint64)v->addr)/PGSIZE;


/*
    //may be a cow page
    if(v->inmemory&(1<<s)){
        printf("a mmap cow page");
        return 1;
    }*/

    pte_t *pte=walk(p->pagetable,addr,0);

    if(pte&&(*pte&PTE_V)){
       if(*pte&PTE_COW){
           return 1;
       }else{
           return -1;
       }
    }

   
    int perm= PTE_U;

    if(v->prot&PROT_READ){
        perm|=PTE_R;
    }
    if(v->prot&PROT_WRITE){
        perm|=PTE_W;
        //printf("set writted at va %p\n",addr);
    }

    if(perm==PTE_U){
        return -1;
    }

    char *mem;
    begin_op();
    ilock(v->f->ip);
    if(v->flags==MAP_PRIVATE){

        if((mem=kalloc())==0){
            iunlock(v->f->ip);
            end_op();
            return -1;
        }
       
        memset(mem,0,PGSIZE);
       
        if(readi(v->f->ip,0,(uint64)mem,addr-(uint64)v->addr+v->off,PGSIZE)==-1){
            iunlock(v->f->ip);
            end_op();
            return -1;
        }
    }else if(v->flags==MAP_SHARED){
         //mem point to the buffer_chche
         struct buf *bp;
         bp=bufgeti(v->f->ip,addr-(uint64)v->addr+v->off);
         mem=(char *)bp->data;
        // printf("load the buffer cache first num:%d",*mem);
         // release the sleeplock
         // avoid destory the bp
         bpin(bp);
         brelse(bp);
    }else{
        panic("mmap:flags");
    }
    iunlock(v->f->ip);
    end_op();
    
    if(mappages(p->pagetable,addr,PGSIZE,(uint64)mem,perm)!=0){
            //end_op();
            return -1;
    }
    v->inmemory|=(1<<s);
    //change in_memory
    
    return 0;
}


int  munmap(uint64 addr,uint len){
    //addr is in  start or end ,len is 0 or the whole file
    // addr must be page aligned
    // we have find the region
    // lenth must be pagealigned

    if(len==0||len>MMAPMAXLENTH||len%PGSIZE!=0){
      return -1;
    }

    struct proc *p=myproc();
    struct vma*vi=0;
    struct vma*v=0;

    int i=0;

    for(;i<NPMMAPVMA;i++){
        if((vi=p->mapregiontable[i])!=0){
            if(addr>=(uint64)(vi->addr)&&addr<(uint64)(vi->addr)+vi->lenth){
                v=vi;
                break;
            }
        }
    }

     if(len%PGSIZE||len>v->lenth){
        // lenth error
        return -1;
    }


    if(v==0||addr+len>v->addr+v->lenth){
        return -1;
    }

    
    // can not handle this situation
    if(addr!=v->addr&&addr+len!=(uint64)v->addr+v->lenth){
        return -1;
    }


    //layzy allocation may not in memory
    uint64 umap_addr=addr;
    //int r=0; 
    // this is very important
    int s=(addr-v->addr)/PGSIZE;



    begin_op();
    ilock(v->f->ip); 
    for(;umap_addr<addr+len;umap_addr+=PGSIZE,s++){
        // check if is in memory
        //int inmemory=(walkaddr(p->pagetable,umap_addr)!=0);
        int inmemory=((v->inmemory&(1<<s))!=0);
        //uint off;
        if(inmemory){
             if(v->flags==MAP_SHARED){

                struct buf*bp;

              
                bp=bufgeti(v->f->ip,umap_addr-(uint64)v->addr+v->off);
               

               
                log_write(bp);
               
                

                bunpin(bp);
                brelse(bp);
                uvmunmap(p->pagetable,umap_addr,1,0);
                

             }else if(v->flags==MAP_PRIVATE){
                uvmunmap(p->pagetable,umap_addr,1,1);
             }
             else{
                 panic("umap:flags");
             }
             v->inmemory&=~(1<<s);
       }

    }
    iunlock(v->f->ip);
    end_op();

    v->lenth-=len;
    if(addr==(uint64)v->addr){
        v->addr+=len;
        v->off+=len;
        v->inmemory=(v->inmemory)>>(len/PGSIZE);
    }

    if(v->lenth==0){
       fileclose(v->f);
       vmafree(v);
       // free the fuck bitbmap
       p->mapregiontable[i]=0;
       int mask=(1<<(addr-MMAPADDR)/MMAPMAXLENTH);
       p->mmapbitmap|=mask;
    }

    return 0;
}


//used in exit
void munmapall(){
    
    struct proc *p=myproc();
    struct vma *v=0;
    for(int i=0;i<NPMMAPVMA;i++){
        if((v=p->mapregiontable[i])!=0){
          munmap(v->addr,v->lenth);
        }
    }
}





