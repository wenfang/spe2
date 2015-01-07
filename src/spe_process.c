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
  int       notifyFd;
} proc_t __attribute__((aligned(sizeof(long))));

// master use this
static proc_t procs[SPE_MAX_PROC];

static int getSlot() {
  for (int i=0; i<SPE_MAX_PROC; i++) {
    if (procs[i].pid == 0) return i;
  }
  return -1;
}

// worker use this
static int controlFd;
static speTask_t controlTask;

/*
===================================================================================================
SpeProcessExit
===================================================================================================
*/
int SpeProcessExit(pid_t pid) {
  for (int i=0; i<SPE_MAX_PROC; i++) {
    if (procs[i].pid == pid) {
      procs[i].pid = 0;
      close(procs[i].notifyFd);
    }
    return 0;
  }
  return -1;
}

/*
===================================================================================================
SpeProcessFork
===================================================================================================
*/
int SpeProcessFork() {
  int procSlot;
  if ((procSlot = getSlot()) == -1) return -1;

  int channel[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, channel)) return -1;

  pid_t pid = fork();
  if (pid < 0) return -1;
  // for worker
  if (!pid) {
    // set cpu affinity
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(procSlot%SpeCpuCount(), &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);
    // set controlFd
    controlFd = channel[1];
    close(channel[0]);
    return 0;
  }
  // for master
  procs[procSlot].pid = pid;
  procs[procSlot].notifyFd = channel[0];
  close(channel[1]);
  return 1;
}

/*
===================================================================================================
SpeProcessEnableControl
===================================================================================================
*/
void
SpeProcessEnableControl(speHandler_t handler) {
  SpeTaskInit(&controlTask, SPE_TASK_FAST);
  controlTask.Handler = handler;
  SpeEpollEnable(controlFd, SPE_EPOLL_READ, &controlTask);
}

/*
===================================================================================================
SpeProcessDisableControl
===================================================================================================
*/
void
SpeProcessDisableControl() {
  SpeEpollDisable(controlFd, SPE_EPOLL_READ);
}
