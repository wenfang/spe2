#ifndef __SPE_TASK_H
#define __SPE_TASK_H

#include "spe_module.h"
#include "spe_list.h"
#include "spe_rbtree.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <stdbool.h>

typedef struct spe_task_s {
  spe_handler_t     _handler;
  struct rb_node    _timer_node;
  struct list_head  _task_node;
  unsigned          _expire;
  unsigned          _status:6;
  unsigned          _fast:1;
  unsigned          timeout:1;
} spe_task_t __attribute__((aligned(sizeof(long))));

extern void
spe_task_init(spe_task_t* task);

static inline void
spe_task_set_handler(spe_task_t* task, spe_handler_t handler, unsigned fast) {
  ASSERT(task);
  task->_handler = handler;
  task->_fast    = fast;
}

extern bool
spe_task_empty(void);

extern void
spe_task_schedule(spe_task_t* task);

extern void
spe_task_schedule_timeout(spe_task_t* task, unsigned long ms);

extern void
spe_task_dequeue(spe_task_t* task);

extern void
spe_task_process(void);

extern spe_module_t spe_task_module;

#endif
