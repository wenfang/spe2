#define _GNU_SOURCE
#include <sched.h>

#include "spe_process.h"
#include "spe_util.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SPE_MAX_PROCESS 128

struct process_s {
  pid_t pid;
  int   channel[2];
};
typedef struct process_s process_t;

static process_t processes[SPE_MAX_PROCESS];

static int processSlot = -1;
static int lastProcess;

static int getSlot() {
  for (int i=0; i<lastProcess; i++) {
    if (processes[i].pid == -1) {
      return i;
    }
  }
  if (lastProcess < SPE_MAX_PROCESS) return lastProcess++;
  return -1;
}

int SpeProcessSpawn(unsigned procs) {
  unsigned cpuCount = SpeCpuCount();
  for (int i=0; i<procs; i++) {
    processSlot = getSlot();
    if (processSlot == -1) return -1;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, processes[processSlot].channel)) {
      processSlot = -1;
      return -1;
    }
    pid_t pid = fork();
    if (!pid) {
      cpu_set_t set;
      CPU_ZERO(&set);
      CPU_SET(processSlot%cpuCount, &set);
      sched_setaffinity(0, sizeof(cpu_set_t), &set);
      close(processes[processSlot].channel[0]);
      return 0;
    } else if (pid < 0) {
      processSlot = -1;
      return -1;
    }
    processes[processSlot].pid = pid;
    close(processes[processSlot].channel[1]);
  }
  processSlot = -1;
  return 1;
}
