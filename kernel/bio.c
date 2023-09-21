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
  
  // init spinlock per bucket
  for(int i=0;i<NBUCKET;i++)
    initlock(&bucketlock[i],"bcache.bucket");

  // insert all buffers into bucket0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
      b->next=table[0].next;
      if(table[0].next){
        table[0].next->prev=b;
      }
      table[0].next=b;
      b->prev=&table[0];
  }

 /* // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }*/
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *free_buf=0;

  int key=blockno%NBUCKET;

  acquire(&bucketlock[key]);

  for(b=table[key].next;b;b=b->next){
     if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucketlock[key]);
      // only one process can use this buffer 
      acquiresleep(&b->lock);
      return b;
    }
     if(b->refcnt==0){
       free_buf=b;
     }
  }

   // don't need to move to another bucket
   if(free_buf){
     goto FIND;
   }
  
   for(int i=0;i<NBUCKET;i++){
     if(i!=key){
        acquire(&bucketlock[i]);
        for(b=table[i].next;b;b=b->next){
            if(b->refcnt==0){
               if(b->next){
                 b->next->prev=b->prev;
               }
               b->prev->next=b->next;
               release(&bucketlock[i]);
               
               free_buf=b;
                
               b->next=table[key].next;
               if(table[key].next){
                 table[key].next->prev=b;
               }
               table[key].next=b;
               b->prev=&table[key];
               goto FIND;
            }
        }
        release(&bucketlock[i]);
     }
   }

  release(&bucketlock[key]);
  panic("bget: no buffers");

  FIND:
     free_buf->dev=dev;
     free_buf->blockno=blockno;
     free_buf->valid=0;
     free_buf->refcnt=1;
     release(&bucketlock[key]);
     acquiresleep(&free_buf->lock);
     return free_buf;
/*

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      // only one process can use this buffer 
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");*/
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
  
  int key=b->blockno%NBUCKET;
  acquire(&bucketlock[key]);
  b->refcnt--;
  release(&bucketlock[key]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


