#ifndef LIST_H
#define LIST_H
#include "types.h"

#define offsetof(t, d) __builtin_offsetof(t, d)

struct list{
  struct list *prev;
  struct list *next;
};

#define offsetof(t, d) __builtin_offsetof(t, d)
#define list_entry(LIST_ELEM, STRUCT, MEMBER)           \
        ((STRUCT *) ((uint8 *) &(LIST_ELEM)->next     \
                     - offsetof (STRUCT, MEMBER.next)))
#endif