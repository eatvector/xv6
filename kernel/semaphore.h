#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include"spinlock.h"

struct semaphore{
  struct spinlock lock;
  int count;
};

#endif