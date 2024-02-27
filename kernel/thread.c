#include"types.h"
#include "defs.h"
#include"spinlock.h"
#include "thread.h"

struct thread thread[]




int nexttid=0;
struct  spinlock  thread_lock;


void threadinit(void){
   initlock(&thread_lock,"thread_lock");
}

int
alloctid()
{
  int tid;
  acquire(&thread_lock);
  tid = nexttid;
  nexttid = nexttid + 1;
  release(&thread_lock);
  return tid;
}


//never user attr in xv6
int thread_create(int *tid,void *attr,void *(start)(void*),void *args){
    // allocate a userthread  stack
    //first call exec we allocate N thread must have a protect page.
    //




}

void thread_exit(void *retval){

}


int thread_join(int tid,void **retval){
  // acquire the spinlock
  //check id in the same process
  // process exit
  //  check the state 
  //if not sleep onit.
}


int thread_self(void){

}


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



