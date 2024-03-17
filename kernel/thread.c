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
  
  for(t= thread; t < &thread[NPROC]; t++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (t - thread));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

static struct thread* allocthread(void){
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
  t->proc=myproc();

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
static void freethread(struct thread *t){
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

   uint64 sp;
   int i;
   for(i=0;i<NPTHREAD;i++){
     
     if(p->usatckbitmap&(1<<i)){
         sp=p->ustack[i];
     }
   }

   if(i==NPTHREAD){
      goto bad;
   }

   //remember id  in thread struct
   newt->ustackid=i;
   newt->isustackalloc=1;
   
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
int  do_thread_join(int tid,int type){
   struct proc *p=myproc();
   struct thread *t=mythread();

   if(!holding(&p->thread_list_lock)){
     panic("Should thread_list lock\n");
   }

  struct list *l=p->thread_list.next;
  struct thread *tt;
     while(l){
      tt=list_entry(l,struct thread,thread_list);
       if(tt->tid==tid){
         break;
       }
       l=l->next;
     }
  release(&p->thread_list);

  if(tt==0)
    return -1;


  //avoid dead lock?

  //may have dead locks.
/*
  if(t->tid<tt->tid){
    acquire(&t->lock);
    acquire(&tt->lock);
  }else{
    acquire(&tt->lock);
    acquire(&t->lock);
  }
*/
  acquire(&tt->lock);

  // the thread tt has been joined.
   if(type&&tt->joined==1){
     return -1;
   }

 // int join_flag=0;

   while(tt->state!=ZOMBIE){
     if(type)
       tt->joined=1;
       /*if(join_flag==0){
         join_flag=1;
         release(&tt->lock);
       }*/
       // tt exit will set tt->joined before wake up t
      // if(threadkilled())
       sleep(tt,&tt->lock); 
  }

    /*if(join_flag==0)
       release(&t->lock);*/

    release(&tt->lock);
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

  acquire(&t->lock);
   t->state=ZOMBIE;
   t->xstate=0;
  //release(&t->lock);

  // this thread is exit,but 
  if(t->joined){
   // wakeup(t);
    t->joined=0;
  }
  release(&t->lock);


  //rember to initial it
  acquire(&p->thread_list_lock);


  //rm from list
  //t->prev->next=t->next;
  //have bugs?
  t->thread_list.next->prev=t->thread_list.prev;
  t->thread_list.prev->next=t->thread_list.next;

  wakeup(t);

  struct thread *tt;
  struct thread *l;

   //is p->mainthread clean?
  if(t==p->mainthread){
    //wait for other thread exit
    l=p->thread_list.next;
    // killthread(tt->tid);
     while(l){
       killthread(tt);
       //thread_join(tt->tid,0);
       //do not check joined file
        //may have bugs?
       do_thread_join(tt->tid,0);

       // have bug
       tt=tt->next;
     }
  }

  //if no 
  //all other thread is exit
  if(p->thread_list.next==0){
      p->xstate=0;
     // p->state=ZOMBIE;(kill the process);
  }

  release(&p->thread_list_lock);
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
  acquire(&p->thread_list_lock);
  return do_thread_join(tid,1);
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



