#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *, uint64, struct inode *, uint, uint);

int flags2perm(int flags)
{
    int perm = 0;
    if(flags & 0x1)
      perm = PTE_X;
    if(flags & 0x2)
      perm |= PTE_W;
    return perm;
}

int
exec(char *path, char **argv)
{

  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG], stackbase;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetable_t pagetable = 0, oldpagetable;
  struct proc *p = myproc();

  begin_op();

  //lost all the map file
  //munmapall();
  //for mmap file do ssomething
  // clear all the vma;
  munmapall();


  //did mmap need to modify like this.
  for(int i=0;i<NPEXECVMA;i++){
    if(p->execvma[i]){
      if(p->execvma[i]->ip){
         iput(p->execvma[i]->ip);
      }
      vmafree(p->execvma[i]);
      p->execvma[i]=0;
    }
  }
  

  //clear the exec vma?

  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);

   

  // 
  int j=0;
  
  // Check ELF header
  if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;

  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pagetable = proc_pagetable(p)) == 0)
    goto bad;

  // Load program into memory.
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    uint64 sz1;

    //remember the file info
    // error handler
    //free the vma
    if(j>=NPEXECVMA){
      goto bad;
    }

    p->execvma[j]=vmaalloc();

    if(p->execvma[j]==0){
      goto bad;
    }
    p->execvma[j]->addr=ph.vaddr;
    p->execvma[j]->ip=idup(ip);
    p->execvma[j]->filesz=ph.filesz;
    p->execvma[j]->memsz=ph.memsz;
    p->execvma[j]->off=ph.off;
    p->execvma[j]->flags=flags2perm(ph.flags)|PTE_R|PTE_U;
    j++;
    


    sz1= ph.vaddr + ph.memsz;
    if(sz1>sz){
      sz=sz1;
    }

  /*
    if((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz, flags2perm(ph.flags))) == 0)
      goto bad;
    sz = sz1;
    if(loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;*/
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  p = myproc();
  uint64 oldsz = p->sz;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible as a stack guard.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  uint64 sz1;
  if((sz1 = uvmalloc(pagetable, sz, sz + 2*PGSIZE, PTE_W)) == 0)
    goto bad;
  sz = sz1;
  uvmclear(pagetable, sz-2*PGSIZE);
  sp = sz;
  stackbase = sp - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  // push the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;
  if(sp < stackbase)
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
    
  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry;  // initial program counter = main
  p->trapframe->sp = sp; // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);


  return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:
  if(pagetable)
    proc_freepagetable(pagetable, sz);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}

//0xc lost instruct 
// when have page fault do this
int exechandler(uint64 va){
  //printf("handle exec at va %x   noff :%d\n",va,mycpu()->noff);



  va=PGROUNDDOWN(va);
   struct proc*p=myproc();
   struct vma*vma;
   struct inode*ip;

   //printf("handle exec at va %x   noff :%d\n",va,mycpu()->noff);
         /* extern int acquirecnt;
          extern int releasecnt;
          //extern char *spinmem;
          extern uint64 pipeacquirecnt;
          extern uint64 pipereleasecnt;

         
          if(acquirecnt!=releasecnt){
            printf("start panic at va %x   acquirecnt %x releasecnt %x\n",va,acquirecnt,releasecnt);
            printf("pipe lock acquire %x release %x\n",pipeacquirecnt,pipereleasecnt);
      
           //printf the lock info

            panic("fffff\n");
          }*/
     

  begin_op();

  for(int i=0;i<NPEXECVMA;i++){
       vma=p->execvma[i];

        
       if(vma){
        if(va>=vma->addr&&va<vma->addr+vma->memsz){
             
             char *mem=kalloc();
             if(mem==0){
               return -1;
             }
             memset(mem,0,PGSIZE);
             //printf("pid %d map page at va %x\n",p->pid,va);
             if(mappages(p->pagetable,va,PGSIZE,(uint64)mem,vma->flags)==-1){
                  goto bad;
             }
             
             ip=vma->ip;
             ilock(ip);
             //i read more data to this file
             //here we have a fucking bug.
             uint sz;
             uint off=vma->off+va-vma->addr;
             if(off<(vma->off+vma->filesz)){
               sz=vma->off+vma->filesz-off;
              
               if(PGSIZE<sz){
                 sz=PGSIZE;
               }
                
                if(va==0){
                  printf("sz is %d\n",sz);
                }
               /* printf("load from off %x sz %x to va %x  noff :%d\n",off,sz,va,mycpu()->noff);
                extern int acquirecnt;
                extern int releasecnt;
                printf("acquire cnt%d release cnt%d\n",acquirecnt,releasecnt);*/
                //acquire one but not release;
               
                if(loadseg(p->pagetable,va,ip,off ,sz)<0){
                  printf("load seg error\n");
                  uvmunmap(p->pagetable,va,1,1);
                  iunlock(ip);
                  goto bad;
                }
             }
          iunlock(ip);
          end_op();
          //extern int acquirecnt;
          //extern int releasecnt;
          /*
          if(acquirecnt!=releasecnt){
            printf("end panic at va %x\n",va);
            panic("fffff\n");
          }*/


          return 0;
        }
       }else{
         break;
       }
  }

  bad:
  end_op();
  return -1;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  //printf("offset is what %x\n",offset);

  for(i = 0; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == 0)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, 0, (uint64)pa, offset+i, n) != n)
      return -1;
  }
  
  return 0;
}
