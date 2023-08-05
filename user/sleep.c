#include "kernel/types.h"
#include "user/user.h"


int main(int argc,char*argv[]){
   if(argc<2){
       fprintf(2,"Error:please use sleep time\n");
       exit(1);
   }else{
       int ticks=atoi(argv[1]);
       if(sleep(ticks)==-1){
           fprintf(2,"Error:sleep fail\n");
           exit(1);
       }
   }
  exit(0);
}