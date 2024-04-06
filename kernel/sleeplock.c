// Sleeping locks

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->tid = 0;
}

void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  while (lk->locked) {
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->tid =mythread()->tid;
  release(&lk->lk);
}


// check kill
int
acquiresleep_kill(struct sleeplock *lk)
{
  acquire(&lk->lk);
  while (lk->locked) {
    // the current thread has been killed
    if(threadkilled(mythread())){
          release(&lk->lk);
          return -1;
    }
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->tid =mythread()->tid;
  release(&lk->lk);
  return 0;
}





void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->tid = 0;
  wakeup(lk);
  release(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked && (lk->tid == mythread()->tid);
  release(&lk->lk);
  return r;
}



