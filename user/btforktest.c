#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(void){

    int ret1=fork();
    //int ret2=fork();
    if(ret1==0){
        sleep(10);
        //return 1;
    }else {
       sleep(10);
       exit(1);
    }

    int ret2=fork();
      if(ret2==0){
        sleep(10);
        exit(1);
    }else {
       sleep(10);
       exit(1);
    }
   
}