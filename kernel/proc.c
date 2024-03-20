#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "sbi.h"

struct thread;

extern struct thread thread[NTHREAD];
struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
/*
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}
*/


// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      initlock(&p->thread_list_lock,"thread_list_lock");
      initmutextlock(&p->vmalock,"vmalock");
      // all is free
      p->mmapbitmap=0xFFFF;
      p->state = UNUSED;
     // p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();// can not disable M mode interrupt
  struct cpu *c = mycpu();
  struct proc *p = c->thread->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  /*if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }*/

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
 /* memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;*/

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
 /* if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;*/
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
 // p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct thread *mainthread)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  ///

  
  if(mappages(pagetable, TRAPFRAME(0), PGSIZE,
              (uint64)(mainthread->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }
  

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  struct proc *p=myproc();
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  //free each thread's trapframe.
  // did i need to do this?
  for(int i=0;i<NTHREAD;i++){
     if(p->trapframebitmap&(1<<i)){
        uvmunmap(pagetable, TRAPFRAME(i), 1, 0);
        p->trapframebitmap&=~(1<<i);
     }
  }
  //uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  struct thread*mainthread=allocthread();
  p->mainthread=mainthread;
  mainthread->proc=p;

  // prepare for the very first "return" from kernel to user.
  mainthread->trapframe->epc = 0;      // user program counter
  mainthread->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  // the first process will set it's work dir at /
  // other progress will fork to set it's work dir at /
  p->cwd = namei("/");

  mainthread->state = RUNNABLE;

  /*
   //for the initcode,we do hack
  p->execvma[0]=vmaalloc();
  p->execvma[0]->addr=0;
  p->execvma[0]->memsz=PGSIZE;
  */

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct thread *nt;

  struct thread*t=mythread();
  struct proc *p = myproc();
   
   

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }

  np->vma[NPMMAPVMA]=vmaalloc();
  if(np->vma[NPMMAPVMA]==0){
    return -1;
  }
  vmacopy(p->vma[NPMMAPVMA],np->vma[NPMMAPVMA]);


 // for mmap  fork
 // we fuck panic in fileclose
 np->mmapbitmap=p->mmapbitmap;

  for(int i=0;i<NPMMAPVMA;i++){
     if(p->vma[i]==0){
       np->vma[i]=0;
     }else{
       np->vma[i]=vmaalloc();
       if(np->vma[i]==0){
          release(&np->lock);
           return -1;
       }
       vmacopy(p->vma[i],np->vma[i]);
     }
  }

  //mmap region
  if(uvmmmapcopy(p->pagetable,np->pagetable,p->vma)==-1){
    for(int i=0;i<NPMMAPVMA;i++){
        if(np->vma[i]){
               vmafree(np->vma[i]);
        }
    }
    release(&np->lock);
    return -1;
  }
 
  //for exec region
  for(int i=NPMMAPVMA+NPHEAPVMA;i<NPVMA;i++){
    if(p->vma[i]==0){
      np->vma[i]=0;
    }else{
       np->vma[i]=vmaalloc();
       if(np->vma[i]==0){
         release(&np->lock);
         return -1;
       }
       vmacopy(p->vma[i],np->vma[i]);
    }
  }
// data ,code ,heap
  np->sz = p->sz;
  
  np->ustack_start=p->ustack_start;
   

  // when to free the ustack.
  int ustacki=USTACKI(p->ustack_start,t->ustack);
  np->usatckbitmap=(1<<ustacki);
  // do we need free the trapframe of other thread?
  int trapframei=TRAPFRAMEI(t->trapframeva);
  np->trapframebitmap=(1<<trapframei);
  
  np->nthread=1;

  // copy saved user registers.
 // *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
//  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  /*acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);*/
// allocate a new thread for this 


//first acquire thread lock,the acquire the proc lock
  if((nt = allocthread()) == 0){
    return -1;
  }
  *nt->trapframe=*t->trapframe;
  nt->trapframe->a0=0;
  nt->proc=np;
  nt->ustack=t->ustack;
  //the last thing to do,set it to runable.
  //release(&nt->lock);

  //to avoid deadlock.
  acquire(&np->lock);
   //np->
   if(p->mainthread==t){
     np->mainthread=nt;
   }
    // only one thread,do not have to acquire the thread_list lock.
  np->thread_list.next=nt;
  //release(&np->lock);
  //acquire(&nt->lock);
  nt->thread_list.prev=&np->thread_list;
  nt->state=RUNNABLE;
  //release(&nt->lock);
  release(&np->lock);
  release(&nt->lock);
  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();
  
  
  if(p == initproc)
    panic("init exiting");


  //how can we sat the p->xcode.
  kill_wait();

  vmafree(p->vma[NPMMAPVMA]);

  // for mmap
  // have bigin_op and end_op bugs.
 //  munmapall();

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
    munmapall();
  for(int i=NPMMAPVMA+NPHEAPVMA;i<NPVMA;i++){
    if(p->vma[i]){
      if(p->vma[i]->ip){
         //ilock(p->execvma[i]->ip);
         iput(p->vma[i]->ip);
         //iunlock(p->execvma[i]->ip);
      }
      vmafree(p->vma[i]);
      p->vma[i]=0;
    }
  }
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  // call exit the proc is set ZOMBIE.
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct thread *t=mythread();
  struct proc *p = myproc();


  // i do dirty work here.
  if(pagefaulthandler(addr,sizeof(p->xstate),1)==-1){
    return -1;
  }

  // to avoid lost wakeup.
  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    // how to solve kill proc.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(t, &wait_lock);  //DOC: wait-sleep
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

