#define _GNU_SOURCE
#include <sched.h>

#include "spe_worker.h"
#include "spe_module.h"
#include "spe_server.h"
#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_signal.h"
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

// command list
#define WORKER_STOP  1

// for master
#define SPE_MAX_WORKER 128

typedef struct {
  pid_t pid;
  int   notifyFd;
} worker_t;

static worker_t workers[SPE_MAX_WORKER];

// for worker
static int controlFd;
static speTask_t controlTask;
static bool workerStop;

// for master
static int getSlot() {
  for (int i=0; i<SPE_MAX_WORKER; i++) {
    if (workers[i].pid == 0) return i;
  }
  return -1;
}

/*
===================================================================================================
SpeWorkerFork
===================================================================================================
*/
int SpeWorkerFork() {
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
  workers[procSlot].pid = pid;
  workers[procSlot].notifyFd = channel[0];
  close(channel[1]);
  return 1;
}

/*
===================================================================================================
SpeWorkerReset
===================================================================================================
*/
int SpeWorkerReset(pid_t pid) {
  for (int i=0; i<SPE_MAX_WORKER; i++) {
    if (workers[i].pid == pid) {
      workers[i].pid = 0;
      close(workers[i].notifyFd);
      return 0;
    }
  }
  return -1;
}

/*
===================================================================================================
SpeWorkerStop
===================================================================================================
*/
void SpeWorkerStop() {
  int comm = WORKER_STOP;
  for (int i=0; i<SPE_MAX_WORKER; i++) {
    if (workers[i].pid == 0) continue;
    write(workers[i].notifyFd, &comm, sizeof(int));
    workers[i].pid = 0;
    close(workers[i].notifyFd);
  }
}

// for worker
static void workerCtrlHandler() {
  int comm;
  int res = read(controlFd, &comm, sizeof(int));
  if (res == 0) { // master exit, worker exit
    workerStop = 1;
    return;
  }
  if (comm == WORKER_STOP) {
    workerStop = 1;
  }
}

void
SpeWorkerProcess() {
	SpeSetProctitle("spe: worker");
  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  SpeSignalRegister(SIGCHLD, SIG_IGN);
  SpeSignalRegister(SIGUSR1, SIG_IGN);
  // init worker
  for (int i = 0; i < speModuleNum; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->initWorker) speModules[i]->initWorker(&cycle);
  }
  for (int i = 0; i < speModuleNum; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->initWorker) speModules[i]->initWorker(&cycle);
  }
  // enable control task
  spe_task_init(&controlTask, SPE_TASK_FAST);
  controlTask.Handler = SPE_HANDLER0(workerCtrlHandler);
  SpeEpollEnable(controlFd, SPE_EPOLL_READ, &controlTask);
  // event loop
  unsigned timeout = 300;
  while (!workerStop) {
    if (speTaskNum) timeout = 0;
    SpeServerPreLoop();
    SpeEpollProcess(timeout);
    SpeServerPostLoop();
    spe_task_process();
    SpeSignalProcess();
  }
  // disable control task
  SpeEpollDisable(controlFd, SPE_EPOLL_READ);
  close(controlFd);
  // exit worker
  for (int i = speModuleNum - 1; i >= 0; i--) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->exitWorker) speModules[i]->exitWorker(&cycle);
  }
  for (int i = speModuleNum - 1; i >= 0; i--) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->exitWorker) speModules[i]->exitWorker(&cycle);
  }
}
