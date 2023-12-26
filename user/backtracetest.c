#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(void){
    sleep(10);
    printf("sleep down\n");
    return 0;
}