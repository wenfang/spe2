#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_module.h"
#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include <stdbool.h>

#define SPE_TASK_NORM 0
#define SPE_TASK_FAST 1

typedef struct spe_task_s {
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
spe_task_schedule(speTask_t* task);

extern bool
spe_task_schedule_timeout(speTask_t* task, unsigned long ms);

extern bool
spe_task_dequeue(speTask_t* task);

extern void
spe_task_process(void);

extern int speTaskNum;

extern speModule_t speTaskModule;

#endif
