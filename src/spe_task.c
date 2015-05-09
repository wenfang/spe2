#include "spe_module.h"
#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdlib.h>
#include <pthread.h>

#define SPE_TASK_FREE   0   // task is alloc not in queue
#define SPE_TASK_TIMER  1   // task in timer queue
#define SPE_TASK_QUEUE  2   // task in running queue
#define SPE_TASK_RUN    3   // task is running

int speTaskNum;             // task num in running queue

static LIST_HEAD(task_head);
static struct rb_root   timer_head;

/*
===================================================================================================
spe_task_init
===================================================================================================
*/
void
spe_task_init(speTask_t* task, unsigned flag) {
  ASSERT(task);
  task->Handler = SPE_HANDLER_NULL;
  task->expire  = 0;
  rb_init_node(&task->timerNode);
  INIT_LIST_HEAD(&task->taskNode);
  task->flag    = flag;
  task->status  = SPE_TASK_FREE;
  task->Timeout = 0;
}

/*
===================================================================================================
SpeTaskEnqueue
===================================================================================================
*/
bool
SpeTaskEnqueue(speTask_t* task) {
  ASSERT(task);
  if (unlikely(task->flag == SPE_TASK_FAST)) {
    SPE_HANDLER_CALL(task->Handler);
    return true;
  }
  if (task->status == SPE_TASK_QUEUE) return false;
  if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timerNode, &timer_head);
    rb_init_node(&task->timerNode);
  }
  list_add_tail(&task->taskNode, &task_head);
  task->status = SPE_TASK_QUEUE;
  speTaskNum++;
  return true;
}

/*
===================================================================================================
SpeTaskDequeue
===================================================================================================
*/
bool
SpeTaskDequeue(speTask_t* task) {
  ASSERT(task);
  if (task->status != SPE_TASK_QUEUE) return false;
  list_del_init(&task->taskNode);
  task->status = SPE_TASK_FREE;
  speTaskNum--;
  return true;
}

/*
===================================================================================================
SpeTaskEnqueueTimer
===================================================================================================
*/
bool
SpeTaskEnqueueTimer(speTask_t* task, unsigned long ms) {
  ASSERT(task);
  if (task->status == SPE_TASK_QUEUE) return false;
  if (task->status == SPE_TASK_TIMER) {
    rb_erase(&task->timerNode, &timer_head);
    rb_init_node(&task->timerNode);
  }
  task->expire  = SpeCurrentTime() + ms;
  task->Timeout = 0;
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    speTask_t* curr = rb_entry(*new, speTask_t, timerNode);
    parent = *new;
    if (task->expire < curr->expire) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->timerNode, parent, new);
  rb_insert_color(&task->timerNode, &timer_head);
  task->status = SPE_TASK_TIMER;
  return true;
}

/*
===================================================================================================
SpeTaskDequeueTimer
===================================================================================================
*/
bool
SpeTaskDequeueTimer(speTask_t* task) {
  ASSERT(task);
  if (task->status != SPE_TASK_TIMER) return false;
  rb_erase(&task->timerNode, &timer_head);
  rb_init_node(&task->timerNode);
  task->status = SPE_TASK_FREE;
  return true;
}

bool
spe_task_dequeue(speTask_t* task) {
  ASSERT(task);
  return true;
}

/*
===================================================================================================
SpeTaskProcess
===================================================================================================
*/
void
SpeTaskProcess(void) {
  // check timer queue
  if (!RB_EMPTY_ROOT(&timer_head)) {
    unsigned long curr_time = SpeCurrentTime();
    // check timer list
    struct rb_node* first = rb_first(&timer_head);
    while (first) {
      speTask_t* task = rb_entry(first, speTask_t, timerNode);
      if (task->expire > curr_time) break;
      ASSERT(task->status == SPE_TASK_TIMER);
      rb_erase(&task->timerNode, &timer_head);
      rb_init_node(&task->timerNode);
      task->Timeout = 1;
      // add to task queue
      list_add_tail(&task->taskNode, &task_head);
      task->status = SPE_TASK_QUEUE;
      speTaskNum++;
      first = rb_first(&timer_head);
    } 
  }
  // run task
  while (speTaskNum) {
    speTask_t* task = list_first_entry(&task_head, speTask_t, taskNode);
    if (!task) break;
    ASSERT(task->status == SPE_TASK_QUEUE);
    list_del_init(&task->taskNode);
    speTaskNum--;
    ASSERT(speTaskNum >= 0);
    task->status = SPE_TASK_RUN;
    SPE_HANDLER_CALL(task->Handler);
    // set task stauts to free
    if (task->status == SPE_TASK_RUN) task->status = SPE_TASK_FREE;
  }
}

static bool
task_module_init(speCycle_t *cycle) {
  timer_head = RB_ROOT;
  return true;
}

speModule_t speTaskModule = {
  "speTask",
  0,
  SPE_CORE_MODULE,
  NULL,
  task_module_init,
  NULL,
  NULL, 
};
