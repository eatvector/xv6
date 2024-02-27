#include"spinlock.h"
#include"semaphore.h"
#include"defs.h"

void initsemaphore(struct semaphore *s,int cnt){
    initlock(&(s->lock),"semaphore");
    s->count=cnt;
}


void sema_p(struct semaphore *s){
    // avoid lost wake up
    acquire(&(s->lock));
    // avooid dead lock
    while(s->count==0)
      sleep(s,&(s->lock));
    s->count--;
    release(&(s->lock));
}


void sema_v(struct semaphore *s){
    acquire(&(s->lock));
    s->count++;
    wakeup(s);
    release(&(s->lock));
}
