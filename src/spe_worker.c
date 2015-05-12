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

typedef struct spe_worker_s {
  pid_t pid;
  int   notify_fd;
} spe_worker_t;

static spe_worker_t spe_workers[SPE_MAX_WORKER];

// for worker
static int control_fd;
static spe_task_t control_task;
static bool worker_stop;

// for master
static int get_slot(void) {
  for (int i=0; i<SPE_MAX_WORKER; i++) {
    if (spe_workers[i].pid == 0) return i;
  }
  return -1;
}

/*
===================================================================================================
spe_worker_fork
===================================================================================================
*/
int spe_worker_fork(void) {
  int proc_slot;
  if ((proc_slot = get_slot()) == -1) return -1;

  int channel[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, channel)) return -1;

  pid_t pid = fork();
  if (pid < 0) return -1;
  // for worker
  if (!pid) {
    // set cpu affinity
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(proc_slot%spe_cpu_count(), &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);
    // set control_fd
    control_fd = channel[1];
    close(channel[0]);
    return 0;
  }
  // for master
  spe_workers[proc_slot].pid        = pid;
  spe_workers[proc_slot].notify_fd  = channel[0];
  close(channel[1]);
  return 1;
}

/*
===================================================================================================
spe_worker_reset
===================================================================================================
*/
int spe_worker_reset(pid_t pid) {
  for (int i=0; i<SPE_MAX_WORKER; i++) {
    if (spe_workers[i].pid == pid) {
      spe_workers[i].pid = 0;
      close(spe_workers[i].notify_fd);
      return 0;
    }
  }
  return -1;
}

/*
===================================================================================================
spe_worker_stop
===================================================================================================
*/
void spe_worker_stop(void) {
  int comm = WORKER_STOP;
  for (int i=0; i<SPE_MAX_WORKER; i++) {
    if (spe_workers[i].pid == 0) continue;
    write(spe_workers[i].notify_fd, &comm, sizeof(int));
    spe_workers[i].pid = 0;
    close(spe_workers[i].notify_fd);
  }
}

// for worker
static void worker_control_handler() {
  int comm;
  int res = read(control_fd, &comm, sizeof(int));
  if (res < 0) return;
  if (res == 0) { // master exit, worker exit
    worker_stop = 1;
    return;
  }
  if (comm == WORKER_STOP) {
    worker_stop = 1;
  }
}

void
spe_worker_process(void) {
	spe_set_proc_title("spe: worker");
  spe_signal_register(SIGPIPE, SIG_IGN);
  spe_signal_register(SIGHUP, SIG_IGN);
  spe_signal_register(SIGCHLD, SIG_IGN);
  spe_signal_register(SIGUSR1, SIG_IGN);
  // init worker
  for (int i = 0; i < spe_module_num; i++) {
    if (spe_modules[i]->module_type != SPE_CORE_MODULE) continue;
    if (spe_modules[i]->init_worker) spe_modules[i]->init_worker(&cycle);
  }
  for (int i = 0; i < spe_module_num; i++) {
    if (spe_modules[i]->module_type != SPE_USER_MODULE) continue;
    if (spe_modules[i]->init_worker) spe_modules[i]->init_worker(&cycle);
  }
  // enable control task
  spe_task_init(&control_task);
  spe_task_set_handler(&control_task, SPE_HANDLER0(worker_control_handler), 1);
  spe_epoll_enable(control_fd, SPE_EPOLL_READ, &control_task);
  // event loop
  unsigned timeout = 300;
  while (!worker_stop) {
    if (spe_task_empty()) timeout = 0;
    SpeServerPreLoop();
    spe_epoll_process(timeout);
    SpeServerPostLoop();
    spe_task_process();
    spe_signal_process();
  }
  // disable control task
  spe_epoll_disable(control_fd, SPE_EPOLL_READ);
  close(control_fd);
  // exit worker
  for (int i = spe_module_num - 1; i >= 0; i--) {
    if (spe_modules[i]->module_type != SPE_USER_MODULE) continue;
    if (spe_modules[i]->exit_worker) spe_modules[i]->exit_worker(&cycle);
  }
  for (int i = spe_module_num - 1; i >= 0; i--) {
    if (spe_modules[i]->module_type != SPE_CORE_MODULE) continue;
    if (spe_modules[i]->exit_worker) spe_modules[i]->exit_worker(&cycle);
  }
}
