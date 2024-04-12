#include"types.h"
#include "defs.h"
#include"spinlock.h"
#include "thread.h"
#include "param.h"
#include "memlayout.h"

struct thread thread[NTHREAD];
int nexttid=0;
struct  spinlock  tid_lock;


struct thread *mythread(){
      //implement this function
  push_off();// can not disable M mode interrupt
  struct cpu *c = mycpu();
  struct thread *p = c->thread;
  pop_off();
  return p;
}


// do not need to hold t->lock
void add_to_threadlist(struct thread *t){
    struct proc *p=myproc();
    if(!holding(&p->thread_list_lock)){
      panic("Should hold process lock\n");
    }
   // acquire(&p->thread_list_lock);
    add_to_list(&p->thread_list,&t->thread_list);
   // release()
}  

void rm_from_threadlist(struct thread *t){
  struct proc *p=myproc();
  if(!holding(&p->thread_list_lock)){
     panic("Should hold process_lock\n");
  } 
  rm_from_list(&t->thread_list);
}

void add_to_exitthreadlist(struct thread *t){
    struct proc *p=myproc();
    if(!holding(&p->exit_thread_list_lock)){
      panic("Should hold process lock\n");
    }
   // acquire(&p->thread_list_lock);
    add_to_list(&p->exit_thread_list,&t->exit_thread_list);

   // release()
}  

void rm_from_exitthreadlist(struct thread *t){
  struct proc *p=myproc();
  if(!holding(&p->exit_thread_list_lock)){
     panic("Should hold process_lock\n");
  } 
  rm_from_list(&t->exit_thread_list);
}






// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{

  // modify to support thread.
  struct thread *t = mythread();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&t->lock);  //DOC: sleeplock1
  // safely release the condition lock.
  release(lk);

  // Go to sleep.
  t->chan = chan;
  t->state = SLEEPING;

  sched();

  // Tidy up.
  t->chan = 0;

  // Reacquire original lock.
  release(&t->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
//be called while holding the condition lock.
void
wakeup(void *chan)
{
  struct thread *t;

  for(t = thread; t < &thread[NPROC]; t++) {
    if(t != mythread()){
      acquire(&t->lock);
      if(t->state == SLEEPING && t->chan == chan) {
        t->state = RUNNABLE;
      }
      release(&t->lock);
    }
  }
}

void
scheduler(void)
{
  struct thread *t;
  struct cpu *c = mycpu();
  
  c->thread = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(t = thread; t < &thread[NTHREAD]; t++) {
      acquire(&t->lock);
      if(t->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        t->state = RUNNING;
        c->thread = t;
        swtch(&c->context, &t->context);
        //release the lock in forkret or after sched.

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->thread = 0;
      }
      release(&t->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct thread *t = mythread();

  if(!holding(&t->lock))
    panic("sched p->lock");

  //may hold more than one lock
  //modify the  exec.


  // avoid dead lock
  if(mycpu()->noff != 1){
   // printf("noff :%d\n",mycpu()->noff);
    panic("sched locks");
  }

  if(t->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&t->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *t = mythread();
  acquire(&t->lock);
  t->state = RUNNABLE;
  // goto scheduler and release the lock.
  sched();
  //have set the pc and sp ,just release the lock.
  release(&t->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  // modify release the thread lock
  release(&mythread()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}


void threadinit(void){
   initlock(&tid_lock,"nexttid");
   struct thread *t=thread;
   for(;t<=&thread[NTHREAD];t++){
     initlock(&t->lock,"thread");
     t->state=UNUSED;
     // allocate ksatck for threads
     t->kstack =  KSTACK((int) (t - thread));
   }
}

int
alloctid()
{
  int tid;
  acquire(&tid_lock);
  tid = nexttid;
  nexttid = nexttid + 1;
  release(&tid_lock);
  return tid;
}

// allocate each thread's kernel stack.
void
thread_mapstacks(pagetable_t kpgtbl)
{
  struct thread *t;
  
  for(t= thread; t < &thread[NPTHREAD]; t++) {
    //never free
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (t - thread));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

//need modify
// the truely physical is in father process 's wait
// may thread may call this .
 uint64 userstackallocate(){
  struct proc*p=myproc();
  if(!holding(&p->lock)){
    panic("Should hold proc lock.");
  }
  //acquire(&p->lock);
  uint ustack=0;
  //int t=1;
  for(int i=0;i<NTHREAD;i++){
    if((p->usatckbitmap&(1<<i))==0){
        // find a free ustack
       //ustack=p->ustack_start+i*2*PGSIZE;
       ustack=USTACK(p->ustack_start,i);
       p->usatckbitmap|=(1<<i);
    }
  }
  //release(&p->lock);
   // can not allocate usertack
   return ustack;
}


// only one thread will calling this so we do not need any lock.
 void userstackfree(uint64 ustack){
   if(ustack%PGSIZE){
     panic("Error ustack");
   }
    struct proc*p=myproc();
   if(!holding(&p->lock)){
    panic("Should hold proc lock.");
  }
   //struct thread 
   //acquire(&p->lock);
   int i=USTACKI(p->ustack_start,ustack);
  // int i=(ustack-p->ustack_start)/PGSIZE;
   if(i<0||i>=NTHREAD){
    panic("Error ustack");
   }
   //mark it unused.
   p->usatckbitmap&=~(1<<i);
  // release(&p->lock);
}

uint64  trapframeallocate(){
  struct proc*p=myproc();
  uint64 trapframeva=0;
  if(!holding(&p->lock)){
    panic("Should hold proc lock.");
  }
  for(int i=0;i<NTHREAD;i++){
    if((p->trapframebitmap&(1<<i))==0){
        // find a free ustack
       //ustack=p->ustack_start+i*2*PGSIZE;
       trapframeva=TRAPFRAME(i);
       //ustack=USTACK(p->ustack_start,i);
       p->trapframebitmap|=(1<<i);
    }
  }
return  trapframeva;                                                                                                          
}

void trapframefree(uint64 trapframeva){
   if(trapframeva%PGSIZE){
     panic("Error trapframeva");
   }
   struct proc *p=myproc();
   int i=TRAPFRAMEI(trapframeva);

   if(!holding(&p->lock)){
    panic("Should hold proc lock.");
  }
  // int i=(ustack-p->ustack_start)/PGSIZE;
   if(i<0||i>=NTHREAD){
    panic("Error ustack");
   }
   //mark it unused.
   p->trapframebitmap&=~(1<<i);
  // release(&p->lock);
}

// how does free proc works?
// free ustack bitsmap
// and ustack
//free trapframe bitsmap
// and trapframe va.
// should be called by thread_joined.
// t->state ZOMBINE no one can acess it.
// what lock do we need to hold.
void freethread(struct thread *t,int unmap){
  struct proc*p=t->proc;
  //give up the userstack
  assert(p);

  //for trapframe
  if(t->trapframe){
    kfree((void *)t->trapframe);
  }

  //have error here?
  if(t->trapframeva){
    // free bit map
    // the page table free will happen in  freeproc.
    trapframefree(t->trapframeva);

    if(unmap){
       uvmunmap(p->pagetable,t->trapframeva,1,0);
    }

    //avoid remap.
    t->trapframeva=0;
  }

 if(t->ustack){
     userstackfree(t->ustack);
     if(unmap){
       // ustack page and guard page
        uvmunmap(p->pagetable,t->ustack-2*PGSIZE,2,1);
     }
     t->ustack=0;
 }
  

  t->tid=0;
  t->killed=0;
  t->joined=0;
  t->proc=0;
  t->killwait=0;
  //t->thread_list=0;
  t->xstate=0;
  t->state=UNUSED; 
}


//return a locked thread.
//state is USED.
struct thread* allocthread(void){
    struct thread *t;
    struct proc *p=myproc();
    //acquire(&p->lock);

    for(t = thread; t< &thread[NTHREAD]; t++) {
      acquire(&t->lock);
      if(t->state == UNUSED) {
        goto found;
      } else {
        release(&t->lock);
      }
    }
    //release(&p->lock);
    return 0;

found:
  t->tid = alloctid();
  t->state = USED;

  // Allocate a trapframe page.
  
  if((t->trapframe= (struct trapframe *)kalloc()) == 0){
   // freeproc(p);
      goto bad;
  }

  //uint64  trapframeva;
  if((t->trapframeva=trapframeallocate())==0){
      kfree(t->trapframe);
      goto bad;
  }

// remap may happened.
  if(mappages(p->pagetable, t->trapframeva, PGSIZE,
              (uint64)(t->trapframe), PTE_R | PTE_W) < 0){
      kfree(t->trapframe);
      goto bad;
  }
/*
  char *usatck_page;
  char *guard_page;

  if((usatck_page=kalloc())==0){
     kfree(t->trapframe);
     goto bad;
  }

   if((guard_page=kalloc())==0){
     kfree(t->trapframe);
     kfree(usatck_page);
     goto bad;
  }

  if((t->ustack=userstackallocate())==0){
     kfree(t->trapframe);
     kfree(usatck_page);
     kfree(guard_page);
    goto bad;
  }

  //mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
  // allocate physiacal page for ustack and guard page.

  if(mappages(p->pagetable,t->ustack-2*PGSIZE,PGSIZE,guard_page,0)==-1){
     kfree(t->trapframe);
     kfree(usatck_page);
     kfree(guard_page);
     goto bad;
  }

  if(mappages(p->pagetable,t->ustack-PGSIZE,PGSHIFT,usatck_page,PTE_R|PTE_W|PTE_U)==-1){
      kfree(t->trapframe);
      kfree(usatck_page);
      kfree(guard_page);
      uvmunmap(p->pagetable,t->ustack-2*PGSIZE,1,1);
      goto bad;
  }
*/

  memset(&t->context, 0, sizeof(t->context));
  t->context.ra = (uint64)forkret;

  //where do we allocate the kstack?
  t->context.sp = t->kstack + PGSIZE;
  //release(&p->lock);
  return t;  //remember that t is locked.

  bad:
    freethread(t,0);
    release(&t->lock);
    //release(&p->lock);
    return 0;
}

static int kill_join_call(){
  int k;
  struct proc *p=myproc();
  acquire(&p->lock);
  k=p->kill_join_call;
  release(&p->lock);
  return k;
}

static void free_threads(){

  struct list *l;
  struct thread*t;
  struct proc *p=myproc();
 acquire(&p->exit_thread_list_lock);
      l=p->exit_thread_list.next;
      while(l){
            t=list_entry(l,struct thread,exit_thread_list);
            rm_from_exitthreadlist(t);
            acquire(&t->lock);
            freethread(t,1);
            release(&t->lock);
            // get the next new thread.
            l=p->exit_thread_list.next;
             
      }
    release(&p->exit_thread_list_lock);
}

//never user attr in xv6
int thread_create(int *tid,void *attr,void *(start)(void*),void *args){
    // allocate a userthread  stack
    //first call exec we allocate N thread must have a protect page.
    
    //kill the old proc,and wait.
    //

   struct proc *p=myproc();
   struct thread *t=mythread();

   struct thread_func tf=*(struct thread_func *)args;
   uint64 threadargs=(uint64)tf.args;

   // do something
   //struct thread   oldt;
  // acquire(&p->lock);
   struct thread *newt=allocthread();

   if(newt==0){
     goto bad;
   }

  newt->proc=p;


  // uint64 sp=userstackallocate();
   pagetable_t pagetable=p->pagetable;

  
  // allocate thread user thread
  char *usatck_page;
  char *guard_page;

  if((usatck_page=kalloc())==0){
     //kfree(t->trapframe);
     goto bad;
  }

   if((guard_page=kalloc())==0){
     kfree(usatck_page);
     goto bad;
  }

  if((newt->ustack=userstackallocate())==0){
     kfree(usatck_page);
     kfree(guard_page);
    goto bad;
  }

  //mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
  // allocate physiacal page for ustack and guard page.

  if(mappages(p->pagetable,newt->ustack-2*PGSIZE,PGSIZE,guard_page,0)==-1){
     kfree(usatck_page);
     kfree(guard_page);
     goto bad;
  }

  if(mappages(p->pagetable,newt->ustack-PGSIZE,PGSHIFT,usatck_page,PTE_R|PTE_W|PTE_U)==-1){
      kfree(usatck_page);
      kfree(guard_page);
      uvmunmap(p->pagetable,newt->ustack-2*PGSIZE,1,1);
      goto bad;
  }

   uint64 sp= newt->ustack;

  //push args to sp
 /* sp-=16;
  if(copyout(pagetable, sp, (char*)threadargs, sizeof(uint64)) < 0)
    goto bad;*/
  
  //set trapframe for this thread. 
   newt->trapframe->a0=(uint64)args;
   newt->trapframe->sp=sp;
   newt->trapframe->epc=(uint64)start;
   release(&newt->lock);
    
   // state is USED

   newt->state=RUNNABLE;

   // here dead lock.
   // lock fuck.
   acquire(&p->thread_list_lock);
   if(kill_join_call(p)){
      release(&p->thread_list_lock);
      acquire(&newt->lock);
      goto bad;
   }
   add_to_threadlist(newt);
   release(&p->thread_list_lock);
 
   acquire(&newt->lock);
   newt->state=RUNNABLE;
   release(&newt->lock);
   return 0;

bad:
//freethread
freethread(newt,1);
release(&newt->lock);
//release(&p->lock);
return -1;
}

//int thread_join(int tid,void **retval);

int killthread(struct thread *thread){
   acquire(&thread->lock);
   thread->killed=1;
   release(&thread->lock);
}

int threadkilled(struct thread *thread){
  int k=0;
  acquire(&thread->lock);
   k=thread->killed;
   release(&thread->lock);
   return k;
}

//must hold thread->lock.
//thread is the thread we join on.

// find a bug here.
int  do_thread_join(struct thread*thread){
   
  struct proc *p=myproc();

  if(!holding(&thread->lock)){
      panic("Should hold thread lock");
  }

  // the thread tt has been joined.
   if(thread->joined==1){
     return -1;
   }

 // int join_flag=0;
 // if the wait thread is keilled,the join will quickly return .
   while(thread->state!=ZOMBIE&&!thread->killwait){
       thread->joined=1;
       // tt exit will set tt->joined before wake up t
      // if(threadkilled())
       sleep(thread,&thread->lock); 
  }

  //if thread state got to ZOMNIE.
  
  if(!thread->killwait){
    // rm from the lsit and free it
      acquire(&p->exit_thread_list_lock);
      rm_from_exit_thread_list(thread);
      release(&p->exit_thread_list_lock);
      freethread(thread,1);
  }

  //  free
  // freethread(thread);


   //assert()
 // release(&tt->lock);
}


static void join_others(){
   
  struct thread *t=mythread();
  struct proc *p=myproc();
  struct thread *tt;
  struct list *l;

   while(1){
      acquire(&p->thread_list_lock);
      l=p->thread_list.next;
      if(l){
        release(&p->thread_list_lock);
        tt=list_entry(l,struct thread,thread_list);
        if(tt!=t){
        acquire(&tt->lock);
        do_thread_join(tt);
        release(&tt->lock);
        }
      }else{
         release(&p->thread_list_lock);
          break;
      }
    }
}


static void  kill_others(){

  struct thread *t=mythread();
  struct proc *p=myproc();
  struct list *l;
  struct thread*tt;


  acquire(&p->thread_list_lock);
  l=p->thread_list.next;
  while(l){
    tt=list_entry(l,struct thread,thread_list);
    if(tt!=t){
      killthread(tt);
    }
  l=l->next;
  }
  release(&p->thread_list_lock);

}


// this function is used in exec and exit.
void kill_join(){

  struct thread *t=mythread();
  struct proc *p=myproc();
  struct thread *tt;
  struct list *l;

  // only one thread will enter this code.
  acquire(&p->lock);
  if(p->kill_join_call){
    release(&p->lock);
    return ;
  }else{
    p->kill_join_call=1;
  }
  release(&p->lock);


  //no new thread will enter the list.

  acquire(&t->lock);
  t->killwait=1;
  t->joined=0;
  //have bugs here?
  wakeup(&t);
  release(&t->lock);

  kill_others();
  join_others();
  free_threads();

 acquire(&t->lock);
  t->killwait=0;
 release(&t->lock);

  acquire(&p->lock);
  p->kill_join_call=0;
  release(&p->lock);
}

// two part atomticly
// kill main thread ,how can we do this?
// the different between 
void thread_exit(uint64 retval){
  struct proc *p=myproc();
  struct thread *t=mythread();  
  struct list *l;
  struct thread*tt;
  assert(p==t->proc);

  struct thread *tt;
  struct list *l;

  //if(t==p->mainthread)

  // rm from one list and add to another list.
  acquire(&p->thread_list_lock);
  rm_from_threadlist(t);
  acquire(&p->exit_thread_list_lock);
  add_to_exit_thread_list(&t->exit_thread_list);
  release(&p->exit_thread_list_lock);
  release(&p->thread_list_lock);


  acquire(&t->lock);
  t->xstate=0;
  t->state=ZOMBIE;
  if(t->joined){
    // join thread will free the resources.
    wakeup(t);
    t->joined=0;
  }
  release(&t->lock);

  if(t==p->mainthread){
      
      //just read code in join_kill
    int flag=0;

    acquire(&p->lock);
    if(p->kill_join_call){
      release(&p->lock);
     }else{
      p->kill_join_call=1;
      flag=1;
    }
    release(&p->lock);

    if(flag){
     join_others();
     // free thread resources that not free.
     free_threads();
  
      acquire(&p->lock);
      p->xstate=0;
      p->state=ZOMBIE;
       //should wake up wait?
      wakeup(p);
      release(&p->lock);
    }
  }

  // when call eixt ,the thread not been join by other thread 
  // just free the thread resources it self
  
 // free the thread resources.
  sched();
  panic("thread exit never reach here\n");
}


//two thread join on the same thread.
int thread_join(int tid,void **retval){
  // acquire the spinlock
  //check id in the same process
  // process exit
  //  check the state 
  //if not sleep onit.

  //never use retval
  struct proc *p=myproc();
  struct list *l;
  struct thread *t=mythread();
  struct thread *tt;

  //acquire(&p->lock);
  //acquire(&p->thread_list);
  // implement a new sleeplock.
   acquire(&p->thread_list_lock);
   l=p->thread_list.next;
    while(l){
      tt=list_entry(l,struct thread,thread_list);
      acquire(&tt->lock);
      // find it and not been joined
       if(tt->tid==tid&&tt->joined==0){
         break;
       }
       release(&tt->lock);
       l=l->next;
     }
    
  
  if(l==0){
    release(&p->thread_list_lock);
    return -1;
  }else {
    // can not wait it self
    if(tt==t){
      release(&tt->lock);
      release(&p->thread_list_lock);
      return -1;
    }
  }
  release(&p->thread_list_lock);

  // we hold tt->lock and p->lock
  int ret=do_thread_join(tt);
  // free the waiting thread.
  //freethread(tt,1);
 // rm_from_threadlist(tt);
  //release(&p->lock);
  release(&tt->lock);
  //release(&p->lock);

  //free the memory
  
  return ret;
}

// kill all other thread ,and wait  they call thread_exit.
// set current thread state to ZOMBIE
// first we acquire thread_list lock,the we can acquire thread lock

// kill and wait all other thread to sate to ZOMBIE.

// three part automatic.


// if kill is 1,send signal to all other thread.



int thread_self(void){

   return mythread()->tid;

}


// now we do not implement these fck this.
int thread_mutex_init(struct pthread_mutex_t *mutex,void *attr){

}

/*uint64 sys_pthread_mutex_destory(void){

}*/

int  thread_mutex_lock(struct pthread_mutex_t *mutex){

}

int  thread_mutex_unlock(struct pthread_mutex_t *mutex){

}

int thread_cond_init(struct pthread_cond_t *cond,void *attr){

}

/*uint64 sys_pthread_mutex_destory(void){

}*/

int  thread_cond_wait(struct pthread_cond_t *cond){

}

int  thread_cond_signal(struct pthread_cond_t *cond){

}



//tlist lock    ->t lock ->p lock ->exit_thread_list_lock