#include "spe.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static int speReapWorker;
static void reapWorker(int sig) {
  speReapWorker = 1;
}

static int speStopWorker;
static void stopWorker(int sig) {
  speStopWorker = 1;
}

static void
SpeMasterProcess() {
  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  SpeSignalRegister(SIGCHLD, reapWorker);
  SpeSignalRegister(SIGUSR1, stopWorker);

  sigset_t set;
  sigemptyset(&set);
  for (;;) {
    sigsuspend(&set);
    SpeSignalProcess();
    if (speReapWorker) {
      for (;;) {
        int status;
        int pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) break;
        if (pid < 0) {
          SPE_LOG_ERR("waipid error");
          break;
        }
        if (SpeWorkerReset(pid) < 0) {
          SPE_LOG_ERR("SpeProcessExit Error");
          continue;
        }
        // for new worker
        int res = SpeWorkerFork();
        if (res < 0) {
          SPE_LOG_ERR("SpeProcessFork Error");
        } else if (res == 0) {
          SpeWorkerProcess();
          return;
        }
        SPE_LOG_WARNING("SpeWorker Restart");
      }
      speReapWorker = 0;
    }
    if (speStopWorker) {
      SPE_LOG_ERR("receive stop");
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    fprintf(stdout, "Usage: %s [configFile]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && !SpeOptCreate(argv[1])) {
    fprintf(stderr, "[ERROR] Config File Parse Error\n");
    return 1;
  }
  // get module number, set module index
  for (int i = 0; speModules[i] != NULL; i++) {
    if (i > SPE_MODULE_MAX) {
      fprintf(stderr, "[ERROR] Too Many Modules\n");
      return 1;
    }
    speModules[i]->index = i;
  }
  // init cycle
  if (!SpeCycleInit()) {
    fprintf(stderr, "[ERROR] SpeCycleInit Error\n");
    return 1;
  }
  // set maxfd
  if (!SpeSetMaxOpenFiles(cycle.maxfd)) {
    fprintf(stderr, "[ERROR] SetMaxOpenFiles %s\n", strerror(errno));
    return 1;
  }
  // daemonize
  if (cycle.daemon) SpeDaemon();
  // init core module
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->initMaster) speModules[i]->initMaster(&cycle);
  }
  // init user module
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->initMaster) speModules[i]->initMaster(&cycle);
  }
  // fork worker
  int res;
  for (int i=0; i<cycle.procs; i++) {
    res = SpeWorkerFork();
    if (res <= 0) break;
  }
  if (res == 0) { // for worker
      SpeWorkerProcess();
  } else if (res == 1) { // for master
      SpeMasterProcess();
  } else {
    fprintf(stderr, "[ERROR] SpeProcessFork Error\n");
  }
  // exit user module
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->exitMaster) speModules[i]->exitMaster(&cycle);
  }
  // exit core module
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->exitMaster) speModules[i]->exitMaster(&cycle);
  }
  
  SpeOptDestroy();
  return 0;
}
