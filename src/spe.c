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
  SpeSavePid(cycle.pidfile);
	SpeSetProctitle("spe: master");

  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  SpeSignalRegister(SIGCHLD, reapWorker);
  SpeSignalRegister(SIGTERM, stopWorker);
  SpeSignalRegister(SIGINT, stopWorker);
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
      SpeWorkerStop();
      break;
    }
  }
  SpeRemovePid(cycle.pidfile);
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
	SpeInitProctitle(argc, argv);
  // init modules index, get speModuleNum
  if (!speModuleInit()) {
    fprintf(stderr, "[ERROR] speModuleInit Error\n");
    return 1;
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
  if (cycle.daemon && SpeDaemon()) {
    fprintf(stderr, "[ERROR] SpeDaemon Error\n");
    return 1;
  }
  // init core module
  for (int i = 0; i < speModuleNum; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->initMaster && !speModules[i]->initMaster(&cycle)) {
      SPE_LOG_ERR("core module initMaster Error");
      return 1;
    }
  }
  // init user module
  for (int i = 0; i < speModuleNum; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->initMaster && !speModules[i]->initMaster(&cycle)) {
      SPE_LOG_ERR("user module initMaster Error");
      return 1;
    }
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
  for (int i = speModuleNum - 1; i >= 0; i--) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->exitMaster && !speModules[i]->exitMaster(&cycle)) {
      SPE_LOG_ERR("user module exitMaster Error");
    }
  }
  // exit core module
  for (int i = speModuleNum - 1; i >= 0; i--) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->exitMaster && !speModules[i]->exitMaster(&cycle)) {
      SPE_LOG_ERR("core module exitMaster Error");
    }
  }
  // destroy speOpt
  SpeOptDestroy();
  return 0;
}
