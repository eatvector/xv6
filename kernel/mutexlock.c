#include"mutexlock.h"

//use semaphore to implement mutexlock.
void initmutextlock(struct mutexlock *mutexlock,char *name){
      mutexlock->name=name;
      initsemaphore(&(mutexlock->s),1);
}

void mutexlock(struct mutexlock *mutexlock){
    sema_p(&(mutexlock->s));
}

void mutexunlock(struct mutexlock *mutexlock){
    sema_v(&(mutexlock->s));
}