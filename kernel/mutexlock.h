#ifndef MUTEXLOCK_H
#define MUTEXLOCK_H
#include "semaphore.h"
#include"defs.h"

struct mutexlock{
   semaphore s;
   char *name;
};

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

#endif
