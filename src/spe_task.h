#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

#define SPE_TASK_NORM 0
#define SPE_TASK_FAST 1

struct speTask_s {
  spe_handler_t     Handler;
  struct rb_node    timerNode;
  struct list_head  taskNode;
  unsigned          expire;
  unsigned          status:6;
  unsigned          flag:1;
  unsigned          Timeout:1;
} __attribute__((aligned(sizeof(long))));
typedef struct speTask_s speTask_t;

extern int speTaskNum;

extern void
SpeTaskInit(speTask_t* task, unsigned flag);

extern bool 
SpeTaskEnqueue(speTask_t* task);

extern bool
SpeTaskDequeue(speTask_t* task);

extern bool
SpeTaskEnqueueTimer(speTask_t* task, unsigned long ms);

extern bool
SpeTaskDequeueTimer(speTask_t* task);

extern void
SpeTaskProcess(void);

extern speModule_t speTaskModule;

#endif
