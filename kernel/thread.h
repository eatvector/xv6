#ifndef THREAD_H
#define THREAD_H
#include "types.h"
#include"spinlock.h"
#include "proc.h"
#include"list.h"
#if 0
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /**< Thread identifier. */
    enum thread_status status;          /**< Thread state. */
    char name[16];                      /**< Name (for debugging purposes). */
    uint8_t *stack;                     /**< Saved stack pointer. */
    int priority;                       /**< Priority. */
    struct list_elem allelem;           /**< List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /**< List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /**< Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /**< Detects stack overflow. */
  };
#endif

enum threadstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };


struct thread{
  

  //struct list;
  struct spinlock lock;//thread lock
  
  struct proc*proc;  //the process the thread belong to
 // struct spinlock proc_lock;  //to protect the shared proc resources
  int tid;        //thread id;
 // int priority;   //for scheduler
  enum threadstate state;// thread status
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  //is killed by other thread?

  struct trapframe *trapframe; // data page for trampoline.S(user thread <--> kernel thread)

  uint64 kstack;               //kernel thread stack
  uint64 ustack;
  struct context context;      // swtch() here to run thread,kernel thread context.

  int xstate;     //  // Exit status of this thread.
 
  
 

  int joined;// a thread can be joined by only one thread
  
  
  
  struct list thread_list;

};


struct thread_func{
  void *(*start)(void *);
  void *args;
};


struct pthread_mutex_t{

};


struct pthread_cond_t{

};
#endif