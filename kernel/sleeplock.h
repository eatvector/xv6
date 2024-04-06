#ifndef SLEEPLOCK_H
#define SLEEPLOCK_H
#include "spinlock.h"
// Long-term locks for processes
struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int tid;           // Process holding lock
};


#endif
