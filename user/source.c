#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };

int main(int argc,char *argv[]){
    if(argc<2){
        fprintf(2,"Error:please use source filename\n");
        exit(1);
    }else{
      int fd;
      // redir 
      close(0);
      if((fd=open(argv[1],O_RDONLY))==-1){
        fprintf(2,"Error:Can not open file %s\n",argv[1]);
        exit(1);
      }
      // sh will read command and exec it from file
      exec("sh",argv);
      fprintf(2,"Error:exec fail\n");
      exit(1);
    }
}