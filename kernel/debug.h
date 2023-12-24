//Debug information

#include"types.h"

struct funcinfo{
   uint64 kfuncaddr;
   uint64 kfuncend;
   uint64 funcname;
};

struct debuginfo{
  struct funcinfo * func;// must be page aligned,do not forget to relaese the memory when process is killed
  uint64 funcsz;
  uint64 strtabaddr;     // must be page aligned
  uint64 strtablen;
  struct  spinlock  lock;
  volatile  int  is_funcinfo_load;
  volatile  int  load;
};


