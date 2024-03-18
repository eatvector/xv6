#include"types.h"
#include "defs.h"
#include"spinlock.h"
#include "thread.h"
#include "param.h"

struct thread thread[NTHREAD];
int nexttid=0;
struct  spinlock  tid_lock;


struct thread *mythread(){
      //implement this function
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

  for(t = proc; t < &thread[NPROC]; t++) {
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
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  // goto scheduler and release the lock.
  sched();
  //have set the pc and sp ,just release the lock.
  release(&p->lock);
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

struct thread* allocthread(void){
    struct thread *t;
    for(t = thread; t< &thread[NTHREAD]; t++) {
      acquire(&t->lock);
      if(t->state == UNUSED) {
        goto found;
      } else {
        release(&t->lock);
      }
    }
    return 0;

found:
  t->tid = alloctid();
  t->state = USED;

  // need modify?
 // t->proc=myproc();

 // t->joined=0;

  // Allocate a trapframe page.
  if((t->trapframe = (struct trapframe *)kalloc()) == 0){
   // freeproc(p);
    release(&t->lock);
    return 0;
  }

  memset(&t->context, 0, sizeof(t->context));
  t->context.ra = (uint64)forkret;

  //where do we allocate the kstack?
  t->context.sp = t->kstack + PGSIZE;
}

//need modify
 uint64 userstackallocate(){
  struct proc*p=myproc();
  acquire(&p->lock);
  uint ustack=0;
  //int t=1;
  for(int i=0;i<NTHREAD;i++){
    if((p->usatckbitmap&(1<<i))==0){
        // find a free ustack
       ustack=p->ustack_start+i*2*PGSIZE;
       //mark it as used
       p->usatckbitmap|=(1<<i);
    }
  }
  release(&p->lock);
   // can not allocate usertack
   return ustack;
}

 void userstackfree(uint64 ustack){
   if(ustack%PGSIZE){
     panic("Error ustack");
   }
   struct proc *p=myproc();
   //struct thread 
   acquire(&p->lock);
   int i=(ustack-p->ustack_start)/PGSIZE;
   if(i<0||i>=NTHREAD){
    panic("Error ustack");
   }
   //mark it unused.
   p->usatckbitmap&=~(1<<i);
   release(&p->lock);
}

void freethread(struct thread *t){
  if(t->trapframe)
    kfree((void *)t->trapframe);
  t->tid=0;
  t->killed=0;
  t->joined=0;
  // not sure here
  //t->priority=0;
  //t->state=UNUSED;
  struct proc*p=t->proc;
  if(p){
    if(t->isustackalloc){
       //add mutex lock
       // other can use it as a ustackva
      p->usatckbitmap&=~(1<<t->ustackid);
    }
    t->isustackalloc=0;
    t->ustackid=0;
  }
  t->proc=0;
  t->state=UNUSED; 
}








//never user attr in xv6
int thread_create(int *tid,void *attr,void *(start)(void*),void *args){
    // allocate a userthread  stack
    //first call exec we allocate N thread must have a protect page.
    
    //kill the old proc,and wait.
    //

   struct proc *p=myproc();
   struct thread_func tf=*(struct thread_func *)args;
   uint64 threadargs=(uint64)tf.args;

   // do something
   //struct thread   oldt;
   struct thread *newt=allocthread();

   if(newt==0){
     goto bad;
   }

  newt->proc=p;


   uint64 sp=userstackallocate();
   newt->ustack=sp;

   if(sp==0){
     goto bad;
   }
   
   
  //push args to sp
  pagetable_t pagetable=p->pagetable;
  sp-=16;
  if(copyout(pagetable, sp, (char*)threadargs, sizeof(uint64)) < 0)
    goto bad;
  

  //set trapframe for this thread. 
   newt->trapframe->a1=sp;
   newt->trapframe->sp=sp;
   newt->trapframe->epc=(uint64)start;

   newt->state=RUNNABLE;

   acquire(&p->lock);
   p->nthread++;
   release(&p->lock);

bad:
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

//must hold lst
int  do_thread_join(struct thread*thread){
   //struct proc *p=myproc();
  // struct thread *t=mythread();
   if(!holding(&thread->lock)){
       panic("Should hold thread lock");
   }
  struct thread *tt=thread;
  // the thread tt has been joined.
   if(&tt->joined==1){
     return -1;
   }

 // int join_flag=0;

   while(tt->state!=ZOMBIE){
       tt->joined=1;
       // tt exit will set tt->joined before wake up t
      // if(threadkilled())
       sleep(tt,&tt->lock); 
  }
   //assert()
 // release(&tt->lock);
}


void thread_exit(uint64 retval){
  // main thread is very special,wait kid process
  //may need to wakeup kid process join it
  //then only exit it self.
  // we do not use retval//
  //assert()

    
  //release the lock we hold.
  //release resources.
  // do we need to free kstack and ustack?

  struct proc *p=myproc();
  struct thread *t=mythread(); 
  assert(p==t->proc);

  //give up the userstack
  userstackfree(t->ustack);
  t->ustack=0;

  acquire(&t->lock);
   t->xstate=0;
  //release(&t->lock);

  // this thread is exit,but 
  if(t->joined){
   // wakeup(t);
    t->joined=0;
  }
   t->state=ZOMBIE;
  release(&t->lock);

  acquire(&p->lock);
  p->nthread--;
  release(&p->lock);


  wakeup(t);
  

  //rember to initial it
  acquire(&p->thread_list_lock);


  //rm from list
  //t->prev->next=t->next;
  //have bugs?
  t->thread_list.next->prev=t->thread_list.prev;
  t->thread_list.prev->next=t->thread_list.next;
 
  struct thread *tt;
  struct list *l;

   //is p->mainthread clean?
  if(t==p->mainthread){
    //wait for other thread exit
    l=p->thread_list.next;

    // killthread(tt->tid);
     while(l){
       tt=list_entry(l,struct thread,thread_list);

       acquire(&tt->lock);
       do_thread_join(tt);
       // have bug
       release(&tt->lock);
       l=l->next;
     }
  }
 release(&p->thread_list_lock);


  //if no 
  //all other thread is exit
  acquire(&p->lock);
  if(p->nthread==0){
    p->xstate=0;
    p->state=ZOMBIE;
  }
  release(&p->lock);

  sched();
  panic("thread exit never reach here\n");
}


int thread_join(int tid,void **retval){
  // acquire the spinlock
  //check id in the same process
  // process exit
  //  check the state 
  //if not sleep onit.

  //never use retval
  struct proc *p=myproc();
  struct list *l=p->thread_list.next;
  acquire(&p->thread_list);
  struct thread *tt;
     while(l){
      tt=list_entry(l,struct thread,thread_list);
      acquire(&tt->lock);
       if(tt->tid==tid){
         break;
       }
       release(&tt->lock);
       l=l->next;
     }
  release(&p->thread_list);

  if(l==0)
    return -1;

  int ret=do_thread_join(tid);

  release(&tt->lock);
}


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



