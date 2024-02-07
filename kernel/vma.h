#ifndef VMA_H
#define VMA_H

#include"types.h"
#include "file.h"

// for mmap
struct vma{
  uint64 addr;

  //for mmap
  uint lenth;//lenth is less then 4,so 4 is enough
  uint off;
  int prot;
  int flags;
  struct file*f;
  int isalloc;
  uint8 inmemory;

  //for exec 
  struct inode *ip;//for exec
  uint64 filesz;// for exec
  uint64 memsz; //for exec

  //for sbrk
};

#define CONSOLE 1
#endif