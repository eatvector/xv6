// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
 // struct buf head;
} bcache;

struct buf  table[NBUCKET];
struct spinlock bucketlock[NBUCKET];

void
binit(void)
{
  struct buf *b;
  
  initlock(&bcache.lock, "bcache");

  for(int i=0;i<NBUCKET;i++){
    table[i].next=&table[i];
    table[i].prev=&table[i];
    initlock(&bucketlock[i],"bcache.bucket");
  }

  // insert all buffers into bucket0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = table[0].next;
    b->prev = &table[0];
    initsleeplock(&b->lock, "buffer");
    table[0].next->prev=b;
    table[0].next=b;
  }

  /*
      b->next=table[0].next;
      if(table[0].next){
        table[0].next->prev=b;
      }
      table[0].next=b;
      b->prev=&table[0];
      initsleeplock(&b->lock, "buffer");
  }*/
}


static struct buf*bget_by_key(uint key,uint dev,uint blockno){
    if(!holding(&bucketlock[key])){
      panic("key_find_free");
    }
   // acquire(&bucketlock[key]);
    struct buf *b;
    struct buf *bfind=0;
    //struct buf *free_buf=0;

    for(b=table[key].next;b!=&table[key];b=b->next){

     if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      bfind=b;
      break;
    }

   /* if(b->refcnt==0){
       free_buf=b;
    }*/
  }


 // find a free buf.
  if(bfind==0){
    for(b=table[key].prev;b!=&table[key];b=b->prev){
       if(b->refcnt==0){
         b->dev=dev;
         b->blockno=blockno;
         b->valid=0;
         b->refcnt=1;
         bfind=b;
         break;
       }
    }
  }

 // release(&bucketlock[key]);
  if(bfind){
    release(&bucketlock[key]);
    acquiresleep(&bfind->lock);
  }
  return bfind;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint key=blockno%NBUCKET;
  acquire(&bucketlock[key]);
  b=bget_by_key(key,dev,blockno);
  if(b){
    return b;
  }
  
  //still hold ing

  // need to steal free buf from other bucket.
  // to avoid dead lock.
  // a acquire key lock 0
  // then want to acquire key lock 1
  // b acquire key lock 1
  //then want to acquire key lock 0

  // relase all the lock we hold;
  release(&bucketlock[key]);

  acquire(&bcache.lock);
  acquire(&bucketlock[key]);

  b=bget_by_key(key,dev,blockno);
  if(b){
     release(&bcache.lock);
     return b;
  }

   for(int i=0;i<NBUCKET;i++){
     if(i!=key){

        acquire(&bucketlock[i]);
        for(b=table[i].prev;b!=&table[i];b=b->prev){
            if(b->refcnt==0){
             b->next->prev=b->prev;
             b->prev->next=b->next;
             release(&bucketlock[i]);
            
              b->next=table[key].next;
              table[key].next->prev=b;
              table[key].next=b;
              b->prev=&table[key];
              goto FIND;
            }
        }
        release(&bucketlock[i]);
     }
   }

// should not arrive here
  release(&bucketlock[key]);
  release(&bcache.lock);
  panic("bget: no buffers");

  FIND:
     b->dev=dev;
     b->blockno=blockno;
     b->valid=0;
     b->refcnt=1;
     release(&bucketlock[key]);
     release(&bcache.lock);
     acquiresleep(&b->lock);
     return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    // read data from disk to buffer
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
 
  uint key=b->blockno%NBUCKET;
  //acquire(&bcache.lock);
  acquire(&bucketlock[key]);
  b->refcnt--;
  if(b->refcnt==0){
    b->next->prev = b->prev;
    b->prev->next = b->next;

    b->next = table[key].next;
    b->prev = &table[key];
    table[key].next->prev = b;
    table[key].next = b;
  }
  release(&bucketlock[key]);
  //release(&bcache.lock);
}

void
bpin(struct buf *b) {
 // acquire(&bcache.lock);
  uint key=b->blockno%NBUCKET;
  acquire(&bucketlock[key]);
  b->refcnt++;
  release(&bucketlock[key]);
}

void
bunpin(struct buf *b) {
  //acquire(&bcache.lock);
  uint key=b->blockno%NBUCKET;
  acquire(&bucketlock[key]);
  b->refcnt--;
  release(&bucketlock[key]);
}


