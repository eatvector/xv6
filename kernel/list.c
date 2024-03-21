#include "list.h"

// do not need to hold t->lock
void add_to_list(struct list *add_pos, struct list*add_obj){
    //struct proc *p=myproc();
    /*if(!holding(&p->thread_list_lock)){
      panic("Should hold thread_list_lock\n");
    }*/

    //we have implement assert
    assert(add_pos);
    assert(add_obj);
    add_obj->next=add_pos->next;

    if(add_pos->next)
    add_pos->next->prev=add_obj;

    add_pos->next=add_obj;
    add_obj->prev=add_pos;
}  

void rm_from_list(struct list*rm_obj){

   assert(rm_obj);
   if(rm_obj->next)
       rm_obj->next->prev=rm_obj->prev;

   if(rm_obj->prev)
      rm_obj->prev->next=rm_obj->next;

    rm_obj->prev=rm_obj->next=0;
}
