#include"defs.h"
#include"types.h"


uint64 sys_thread_create(void){
    // for now we just doing this
     return thread_create(0,0,0,0);
}


uint64 sys_thread_exit(void){
    
     thread_exit(0);
     return 0;
}


uint64 sys_thread_self(void){
    return 0;
}


uint64 sys_thread_join(void){
    return 0;
}

uint64 sys_thread_mutex_init(void){
    return 0;
}

/*uint64 sys_pthread_mutex_destory(void){

}*/

uint64  sys_thread_mutex_lock(void){
    return 0;
}

uint64  sys_thread_mutex_unlock(void){
     return 0;
}

uint64 sys_thread_cond_init(void){
     return 0;
}

/*uint64 sys_pthread_mutex_destory(void){

}*/

uint64  sys_thread_cond_wait(void){
      return 0;
}

uint64  sys_thread_cond_signal(void){
       return 0;
}



