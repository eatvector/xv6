#ifndef MUTEXLOCK_H
#define MUTEXLOCK_H
#include "semaphore.h"
#include"defs.h"

struct mutexlock{
  struct  semaphore s;
   char *name;
};



#endif
