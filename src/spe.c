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
      exit(0);
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    fprintf(stdout, "Usage: %s [configFile]\n", argv[0]);
    return 1;
  }
  if (argc == 2) {
    cycle.confFile = argv[1];
    SpeOptCreate(cycle.confFile);
  }
  if (!SpeSetMaxOpenFiles(MAX_FD)) {
    fprintf(stderr, "[ERROR] SetMaxOpenFiles %s\n", strerror(errno));
    return 1;
  }
  // get module number
  int speModuleNum = 0;
  for (int i = 0; speModules[i] != NULL; i++) {
    speModules[i]->index = i;
    speModuleNum++;
  }
  // init cycle
  if (!SpeCycleInit(speModuleNum)) {
    fprintf(stderr, "[ERROR] SpeCycleInit Error\n");
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

  SpeCycleDestroy(&cycle);
  SpeOptDestroy();
  return 0;
}
