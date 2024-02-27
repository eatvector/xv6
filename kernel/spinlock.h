#ifndef SPINLOCK_H
#define SPINLOCK_H
// how to fix it.
//#include"proc.h"



// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};
#endif
