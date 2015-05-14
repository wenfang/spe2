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
  task->_handler = SPE_HANDLER_NULL;
  task->_expire  = 0;
  rb_init_node(&task->_timer_node);
  INIT_LIST_HEAD(&task->_task_node);
  task->_fast   = 0;
  task->_status = SPE_TASK_FREE;
  task->timeout = 0;
}

/*
===================================================================================================
spe_task_empty
===================================================================================================
*/
bool
spe_task_empty(void) {
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
  if (task->_status == SPE_TASK_QUEUE) return;
  if (task->_status == SPE_TASK_TIMER) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
  }
  if (unlikely(task->_fast)) {
    task->_status = SPE_TASK_FREE;
    SPE_HANDLER_CALL(task->_handler);
    return;
  }
  list_add_tail(&task->_task_node, &task_head);
  task->_status = SPE_TASK_QUEUE;
}

/*
===================================================================================================
spe_task_schedule_timeout
===================================================================================================
*/
void
spe_task_schedule_timeout(spe_task_t* task, unsigned long ms) {
  ASSERT(task);
  if (task->_status == SPE_TASK_QUEUE) return;
  if (task->_status == SPE_TASK_TIMER) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
  }
  task->_expire  = spe_current_time() + ms;
  task->timeout = 0;
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    spe_task_t* curr = rb_entry(*new, spe_task_t, _timer_node);
    parent = *new;
    if (task->_expire < curr->_expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->_timer_node, parent, new);
  rb_insert_color(&task->_timer_node, &timer_head);
  task->_status = SPE_TASK_TIMER;
}

/*
===================================================================================================
spe_task_dequeue
===================================================================================================
*/
void
spe_task_dequeue(spe_task_t* task) {
  ASSERT(task);
  if (task->_status == SPE_TASK_FREE) return;
  if (task->_status == SPE_TASK_QUEUE) {
    list_del_init(&task->_task_node);
  } else if (task->_status == SPE_TASK_TIMER) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
  }
  task->_status = SPE_TASK_FREE;
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
      spe_task_t* task = rb_entry(first, spe_task_t, _timer_node);
      if (task->_expire > curr_time) break;
      ASSERT(task->_status == SPE_TASK_TIMER);
      rb_erase(&task->_timer_node, &timer_head);
      rb_init_node(&task->_timer_node);
      task->timeout = 1;
      // add to task queue
      list_add_tail(&task->_task_node, &task_head);
      task->_status = SPE_TASK_QUEUE;
      first = rb_first(&timer_head);
    } 
  }
  // run task
  while (!list_empty(&task_head)) {
    spe_task_t* task = list_first_entry(&task_head, spe_task_t, _task_node);
    if (!task) break;
    ASSERT(task->_status == SPE_TASK_QUEUE);
    list_del_init(&task->_task_node);
    task->_status = SPE_TASK_FREE;
    SPE_HANDLER_CALL(task->_handler);
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
