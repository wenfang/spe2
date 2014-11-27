#define _GNU_SOURCE
#include <sched.h>

#include "spe_process.h"
#include "spe_task.h"
#include "spe_util.h"
#include "spe_sock.h"
#include "spe_epoll.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SPE_MAX_PROC 128

typedef struct {
  pid_t     pid;
  int       channel[2];
  speTask_t controlTask;
} proc_t __attribute__((aligned(sizeof(long))));

static proc_t procs[SPE_MAX_PROC];

static int procSlot = -1;
static int lastProc;

static int getSlot() {
  for (int i=0; i<lastProc; i++) {
    if (procs[i].pid == -1) {
      return i;
    }
  }
  if (lastProc < SPE_MAX_PROC) return lastProc++;
  return -1;
}

/*
===================================================================================================
SpeProcessSpawn
===================================================================================================
*/
int SpeProcessSpawn(unsigned procNum) {
  unsigned cpuCount = SpeCpuCount();
  for (int i=0; i<procNum; i++) {
    if ((procSlot = getSlot()) == -1) return -1;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, procs[procSlot].channel)) {
      procSlot = -1;
      return -1;
    }
    pid_t pid = fork();
    if (!pid) {
      // set cpu affinity
      cpu_set_t set;
      CPU_ZERO(&set);
      CPU_SET(procSlot%cpuCount, &set);
      sched_setaffinity(0, sizeof(cpu_set_t), &set);
      close(procs[procSlot].channel[0]);
      return 0;
    } else if (pid < 0) {
      procSlot = -1;
      return -1;
    }
    procs[procSlot].pid = pid;
    close(procs[procSlot].channel[1]);
  }
  procSlot = -1;
  return 1;
}

/*
===================================================================================================
SpeProcessFork
===================================================================================================
*/
int SpeProcessFork() {
  if ((procSlot = getSlot()) == -1) return -1;
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, procs[procSlot].channel)) {
    procSlot = -1;
    return -1;
  }

  pid_t pid = fork();
  if (!pid) { // for child
    // set cpu affinity
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(procSlot%SpeCpuCount(), &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);
    close(procs[procSlot].channel[0]);
    return 0;
  } else if (pid < 0) {
    procSlot = -1;
    return -1;
  }
  // for parent
  procs[procSlot].pid = pid;
  close(procs[procSlot].channel[1]);
  procSlot = -1;
  return 1;
}

/*
===================================================================================================
SpeProcessEnableControl
===================================================================================================
*/
void
SpeProcessEnableControl(speHandler_t handler) {
  SpeTaskInit(&procs[procSlot].controlTask, SPE_TASK_FAST);
  procs[procSlot].controlTask.Handler = handler;
  SpeEpollEnable(procs[procSlot].channel[1], SPE_EPOLL_READ, &procs[procSlot].controlTask);
}

/*
===================================================================================================
SpeProcessDisableControl
===================================================================================================
*/
void
SpeProcessDisableControl() {
  SpeEpollDisable(procs[procSlot].channel[1], SPE_EPOLL_READ);
}
