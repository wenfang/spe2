#include "spe_task.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

#define SPE_TASK_FREE   0   // task is alloc not in queue
#define SPE_TASK_TIMER  1   // task in timer queue
#define SPE_TASK_QUEUE  2   // task in running queue

static LIST_HEAD(task_head);
static struct rb_root   timer_head;

/*
===================================================================================================
spe_task_init
===================================================================================================
*/
void
spe_task_init(spe_task_t* task) {
  ASSERT(task);
  task->handler = SPE_HANDLER_NULL;
  task->expire  = 0;
  rb_init_node(&task->timer_node);
  INIT_LIST_HEAD(&task->task_node);
  task->fast    = 0;
  task->status  = SPE_TASK_FREE;
  task->timeout = 0;
}

/*
===================================================================================================
spe_task_empty
===================================================================================================
*/
bool
spe_task_empty() {
  return list_empty(&task_head) ? true : false;
}

/*
===================================================================================================
spe_task_schedule
===================================================================================================
*/
void
spe_task_schedule(spe_task_t* task) {
  ASSERT(task);
  if (task->status == SPE_TASK_QUEUE) return;
  if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timer_node, &timer_head);
    rb_init_node(&task->timer_node);
  }
  if (unlikely(task->fast)) {
    task->status = SPE_TASK_FREE;
    SPE_HANDLER_CALL(task->handler);
    return;
  }
  list_add_tail(&task->task_node, &task_head);
  task->status = SPE_TASK_QUEUE;
}

/*
===================================================================================================
spe_task_schedule_timeout
===================================================================================================
*/
void
spe_task_schedule_timeout(spe_task_t* task, unsigned long ms) {
  ASSERT(task);
  if (task->status == SPE_TASK_QUEUE) return;
  if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timer_node, &timer_head);
    rb_init_node(&task->timer_node);
  }
  task->expire  = spe_current_time() + ms;
  task->timeout = 0;
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    spe_task_t* curr = rb_entry(*new, spe_task_t, timer_node);
    parent = *new;
    if (task->expire < curr->expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->timer_node, parent, new);
  rb_insert_color(&task->timer_node, &timer_head);
  task->status = SPE_TASK_TIMER;
}

/*
===================================================================================================
spe_task_dequeue
===================================================================================================
*/
void
spe_task_dequeue(spe_task_t* task) {
  ASSERT(task);
  if (task->status == SPE_TASK_FREE) return;
  if (task->status == SPE_TASK_QUEUE) {
    list_del_init(&task->task_node);
  } else if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timer_node, &timer_head);
    rb_init_node(&task->timer_node);
  }
  task->status = SPE_TASK_FREE;
}

/*
===================================================================================================
spe_task_process
===================================================================================================
*/
void
spe_task_process(void) {
  // check timer queue
  if (!RB_EMPTY_ROOT(&timer_head)) {
    unsigned long curr_time = spe_current_time();
    // check timer list
    struct rb_node* first = rb_first(&timer_head);
    while (first) {
      spe_task_t* task = rb_entry(first, spe_task_t, timer_node);
      if (task->expire > curr_time) break;
      ASSERT(task->status == SPE_TASK_TIMER);
      rb_erase(&task->timer_node, &timer_head);
      rb_init_node(&task->timer_node);
      task->timeout = 1;
      // add to task queue
      list_add_tail(&task->task_node, &task_head);
      task->status = SPE_TASK_QUEUE;
      first = rb_first(&timer_head);
    } 
  }
  // run task
  while (!list_empty(&task_head)) {
    spe_task_t* task = list_first_entry(&task_head, spe_task_t, task_node);
    if (!task) break;
    ASSERT(task->status == SPE_TASK_QUEUE);
    list_del_init(&task->task_node);
    task->status = SPE_TASK_FREE;
    SPE_HANDLER_CALL(task->handler);
  }
}

static bool
task_module_init(spe_cycle_t *cycle) {
  timer_head = RB_ROOT;
  return true;
}

spe_module_t spe_task_module = {
  "spe_task",
  0,
  SPE_CORE_MODULE,
  NULL,
  task_module_init,
  NULL,
  NULL, 
};
