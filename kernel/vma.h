#ifndef VMA_H
#define VMA_H

#include"types.h"
#include "file.h"

// for mmap
struct vma{
  uint64 addr;
  uint lenth;//lenth is less then 4,so 4 is enough
  uint off;
  int prot;
  int flags;
  struct file*f;
  int isalloc;
  uint8 inmemory;
};

#define CONSOLE 1
#endif