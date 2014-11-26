#include "spe.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

static int speReap;
static void reapWorker(int sig) {
  speReap = 1;
}

static void
SpeMasterProcess() {
  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  SpeSignalRegister(SIGCHLD, reapWorker);

  sigset_t set;
  sigemptyset(&set);
  for (;;) {
    sigsuspend(&set);
    SpeSignalProcess();
    if (speReap) {
      for (;;) {
        int status;
        if (wait(&status) == -1) break;
      }
      speReap = 0;
     break;
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
  // get module number
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
    if (speModules[i]->initMaster) {
      speModules[i]->initMaster(&cycle);
    }
  }
  // init user module
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->initMaster) {
      speModules[i]->initMaster(&cycle);
    }
  }
  // fork child
  int res = SpeProcessSpawn(cycle.procs);
  if (res == 0) {
    SpeWorkerProcess();
  } else {
    SpeMasterProcess();
  }

  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->exitMaster) {
      speModules[i]->exitMaster(&cycle);
    }
  }
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->exitMaster) {
      speModules[i]->exitMaster(&cycle);
    }
  }
  
  SpeOptDestroy();
  return 0;
}
