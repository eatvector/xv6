#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/param.h"


//if a is 0,complier will find the error,else it will do nothing
#ifndef static_assert
#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)
#endif

#define NINODES 200

// Disk layout:
// [ boot block | sb block | log | inode blocks | free bit map | data blocks ]


//to map 2000 block,how many bit mao block do we need
int nbitmap = FSSIZE/(BSIZE*8) + 1;

// to hold 200 indodes ,how many block do we need
int ninodeblocks = NINODES / IPB + 1;
int nlog = LOGSIZE;
int nmeta;    // Number of meta blocks (boot, sb, nlog, inode, bitmap)
int nblocks;  // Number of data blocks

int fsfd;
struct superblock sb;
char zeroes[BSIZE];
uint freeinode = 1;
uint freeblock;


void balloc(int);
void wsect(uint, void*);
void winode(uint, struct dinode*);
void rinode(uint inum, struct dinode *ip);
void rsect(uint sec, void *buf);
uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);
void die(const char *);

// convert to riscv byte order
ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

int bcompare(char *a,char *b){
  for(int i=0;i<BSIZE;i++){
     if(a[i]!=b[i]){
       return 0;
     }
  }
  return 1;
}



#define MIN(a,b)  ((a)>(b)?(b):(a))

void test_kernel(uint kinum){

  int fd=open("kernel/kernel",O_RDONLY);
  struct dinode din;
  // read the kernel inode
  rinode(kinum, &din);
  uint size=xint(din.size);
  char block[BSIZE];
  char qemu_block[BSIZE];
  uint off=0;
  uint readn=0;

  //direct block
  for(int i=0;i<NDIRECT;i++){
    rsect(xint(din.addrs[i]),qemu_block);

    lseek(fd,off,SEEK_SET);
    readn=MIN(BSIZE,size-off);
    memset(block,0,BSIZE);
    if(read(fd,block,readn)!=readn){
      printf("read fail\n");
      return ;
    }


    if(bcompare(block,qemu_block)==0){
        printf("direct block not match\n");
        assert(0);
    }
    off+=readn;
    if(off==size){
      return ;
    }
  }

  // indirect block
  uint indirect[BSIZE/sizeof(uint)];
  rsect(xint(din.addrs[NDIRECT]),indirect);

  for(int i=0;i<NINDIRECT;i++){
    rsect(xint(indirect[i]),qemu_block);

    lseek(fd,off,SEEK_SET);
    readn=MIN(BSIZE,size-off);
    memset(block,0,BSIZE);
    if(read(fd,block,readn)!=readn){
      printf("read fail\n");
      return ;
    }

    if(bcompare(block,qemu_block)==0){
        printf("indirect block not match  %d\n",i);
       // assert(0);
       return;
    }
    off+=readn;
     if(off==size){
      return ;
    }
  }

  //double-indirect block
   rsect(xint(din.addrs[NDIRECT+1]),indirect);


   for(int i=0;i<NINDIRECT;i++){

     uint buf[BSIZE/sizeof(uint)];
     rsect(xint(indirect[i]),buf);

      for(int j=0;j<NINDIRECT;j++){
        rsect(xint(buf[j]),qemu_block);

        lseek(fd,off,SEEK_SET);
        readn=MIN(BSIZE,size-off);
        memset(block,0,BSIZE);

       if(read(fd,block,readn)!=readn){
          printf("read fail\n");
          return ;
        }

       if(bcompare(block,qemu_block)==0){
          printf("doubleindirect block not match\n");
         assert(0);
       }
        off+=readn;
        if(off==size){
          return ;
        }
      }
   }

}

int
main(int argc, char *argv[])
{
  int i, cc, fd;
  uint rootino, inum, off;
  struct dirent de;
  char buf[BSIZE];
  struct dinode din;


  //
  int test=0;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!");

  if(argc < 2){
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }

  assert((BSIZE % sizeof(struct dinode)) == 0);
  assert((BSIZE % sizeof(struct dirent)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0)
    die(argv[1]);
  
  // 1 fs block = 1 disk sector
  nmeta = 2 + nlog + ninodeblocks + nbitmap;
  nblocks = FSSIZE - nmeta;


  //initail super block
  sb.magic = FSMAGIC;
  sb.size = xint(FSSIZE);
  sb.nblocks = xint(nblocks);
  sb.ninodes = xint(NINODES);
  sb.nlog = xint(nlog);
  sb.logstart = xint(2);
  sb.inodestart = xint(2+nlog);
  sb.bmapstart = xint(2+nlog+ninodeblocks);

  printf("nmeta %d (boot, super, log blocks %u inode blocks %u, bitmap blocks %u) blocks %d total %d\n",
         nmeta, nlog, ninodeblocks, nbitmap, nblocks, FSSIZE);

  freeblock = nmeta;     // the first free block that we can allocate

  
  //set the first FSSIZE Blocks to 
  for(i = 0; i < FSSIZE; i++)
    wsect(i, zeroes);

  // write the super block to disk img
  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);


// the first inode point the root dir
  rootino = ialloc(T_DIR);
  assert(rootino == ROOTINO);

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, ".");
  iappend(rootino, &de, sizeof(de));

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(rootino, &de, sizeof(de));


   

  for(i = 2; i < argc; i++){
    // get rid of "user/"
   // printf("file to disk %s\n",argv[i]);
    char *shortname;
    if(strncmp(argv[i], "user/", 5) == 0)
      shortname = argv[i] + 5;
   else if(strncmp(argv[i], "kernel/", 7) == 0){
        test=1;
       shortname=argv[i]+7;
   }
    else
      shortname = argv[i];
    
    assert(index(shortname, '/') == 0);

    if((fd = open(argv[i], 0)) < 0)
      die(argv[i]);

    // Skip leading _ in name when writing to file system.
    // The binaries are named _rm, _cat, etc. to keep the
    // build operating system from trying to execute them
    // in place of system binaries like rm and cat.
    if(shortname[0] == '_')
      shortname += 1;

    inum = ialloc(T_FILE);

    // wite the file dir  to disk image
    bzero(&de, sizeof(de));
    de.inum = xshort(inum);
    strncpy(de.name, shortname, DIRSIZ);
    iappend(rootino, &de, sizeof(de));

   //write the file to disk image
    while((cc = read(fd, buf, sizeof(buf))) > 0)
      iappend(inum, buf, cc);


    close(fd);

    if(test){

      test_kernel(inum);
      test=0;
    }
  }

  // fix size of root inode dir
  //root dir may contain empty entry
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off/BSIZE) + 1) * BSIZE;
  din.size = xint(off);
  winode(rootino, &din);

  //write the bitmap
  balloc(freeblock);

  exit(0);
}


//write buf(BSIZE) to disk file at sec
void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE)
    die("lseek");
  if(write(fsfd, buf, BSIZE) != BSIZE)
    die("write");
}



//write *ip to inode inum
void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE)
    die("lseek");
  if(read(fsfd, buf, BSIZE) != BSIZE)
    die("read");
}


//alloc a new inode and write it to disk
uint
ialloc(ushort type)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  winode(inum, &din);
  return inum;
}


//use a new free sec ,and set it used in bitmap
void
balloc(int used)
{
  uchar buf[BSIZE];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  // bit map only use one block?
  assert(used < BSIZE*8);
  bzero(buf, BSIZE);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %d\n", sb.bmapstart);
  wsect(sb.bmapstart, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

//write xp to file marked by inode inum
// need to modify this
// 



// not change bit map
void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BSIZE;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    } else if(fbn<NDIRECT+NINDIRECT){
      if(xint(din.addrs[NDIRECT]) == 0){
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      //0
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      // x is the section no
      x = xint(indirect[fbn-NDIRECT]);
    }else if(fbn<NDIRECT+NINDIRECT+NDOUBLEINDIRCT){
        printf("block\n");
       // we have some bug here\n
       //uint bn=fbn-NDIRECT-NINDIRECT;
       if(xint(din.addrs[NDIRECT+1])==0){
          din.addrs[NDIRECT+1] = xint(freeblock++);
       }

        uint bn=fbn-NDIRECT-NINDIRECT;
      //1
        rsect(xint(din.addrs[NDIRECT+1]), (char*)indirect);
        uint addr0;
        uint which=bn/NINDIRECT;

       if((addr0=indirect[which])==0){
         addr0=indirect[which]=xint(freeblock++);
         wsect(xint(din.addrs[NDIRECT+1]), (char*)indirect);
       }
       //0

       rsect(xint(addr0), (char*)indirect);
        which=bn%NINDIRECT;
       if((x=indirect[which])==0){
         x=indirect[which]=xint(freeblock++);
         wsect(xint(addr0), (char*)indirect);
       }

    }else{
      // should not reach here
        uint bn=fbn-NDIRECT-NINDIRECT-NDOUBLEINDIRCT;
         if(xint(din.addrs[NDIRECT+2])==0){
          din.addrs[NDIRECT+2] = xint(freeblock++);
       }
       //2
        rsect(xint(din.addrs[NDIRECT+2]), (char*)indirect);
        uint addr1;
        uint which=bn/(NINDIRECT*NINDIRECT);
   
        if((addr1=indirect[which])==0){
         addr1=indirect[which]=xint(freeblock++);
         wsect(xint(din.addrs[NDIRECT+2]), (char*)indirect);
       }

       //1
        rsect(xint(addr1), (char*)indirect);
        uint addr0;
        uint bnn=bn-which*NINDIRECT*NINDIRECT;
        which=(bnn)/NINDIRECT;
        if((addr0=indirect[which])==0){
          addr0=indirect[which]=xint(freeblock++);
          wsect(xint(addr1), (char*)indirect);
        }

        //0
         rsect(xint(addr0), (char*)indirect);
         which=(bnn)%NINDIRECT;
          if((x=indirect[which])==0){
            x=indirect[which]=xint(freeblock++);
            wsect(xint(addr0), (char*)indirect);
        }
    }
    //how many bits we can write this time
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}

void
die(const char *s)
{
  perror(s);
  exit(1);
}
