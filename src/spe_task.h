#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_module.h"
#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

#define SPE_TASK_NORM 0
#define SPE_TASK_FAST 1

typedef struct speTask_s {
  speHandler_t      Handler;
  struct rb_node    timerNode;
  struct list_head  taskNode;
  unsigned          expire;
  unsigned          status:6;
  unsigned          flag:1;
  unsigned          Timeout:1;
} speTask_t __attribute__((aligned(sizeof(long))));

extern void
spe_task_init(speTask_t* task, unsigned flag);

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

extern int speTaskNum;

extern speModule_t speTaskModule;

#endif
