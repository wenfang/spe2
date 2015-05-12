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
  spe_save_pid(cycle.pidfile);
	spe_set_proc_title("spe: master");

  spe_signal_register(SIGPIPE, SIG_IGN);
  spe_signal_register(SIGHUP, SIG_IGN);
  spe_signal_register(SIGCHLD, reapWorker);
  spe_signal_register(SIGTERM, stopWorker);
  spe_signal_register(SIGINT, stopWorker);
  spe_signal_register(SIGUSR1, stopWorker);

  sigset_t set;
  sigemptyset(&set);
  for (;;) {
    sigsuspend(&set);
    spe_signal_process();
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
  spe_remove_pid(cycle.pidfile);
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    fprintf(stdout, "Usage: %s [configFile]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && !spe_opt_create(argv[1])) {
    fprintf(stderr, "[ERROR] Config File Parse Error\n");
    return 1;
  }
	spe_init_proc_title(argc, argv);
  // init modules index, get speModuleNum
  if (!spe_module_init()) {
    fprintf(stderr, "[ERROR] spe_module_init\n");
    return 1;
  }
  // init cycle
  if (!spe_cycle_init()) {
    fprintf(stderr, "[ERROR] spe_cycle_init\n");
    return 1;
  }
  // set maxfd
  if (!spe_max_open_files(cycle.maxfd)) {
    fprintf(stderr, "[ERROR] spe_max_open_files %s\n", strerror(errno));
    return 1;
  }
  // daemonize
  if (cycle.daemon && spe_daemon()) {
    fprintf(stderr, "[ERROR] spe_daemon\n");
    return 1;
  }
  // init core module
  for (int i = 0; i < spe_module_num; i++) {
    if (spe_modules[i]->module_type != SPE_CORE_MODULE) continue;
    if (spe_modules[i]->init_master && !spe_modules[i]->init_master(&cycle)) {
      SPE_LOG_ERR("core module init_master Error");
      return 1;
    }
  }
  // init user module
  for (int i = 0; i < spe_module_num; i++) {
    if (spe_modules[i]->module_type != SPE_USER_MODULE) continue;
    if (spe_modules[i]->init_master && !spe_modules[i]->init_master(&cycle)) {
      SPE_LOG_ERR("user module init_master Error");
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
  for (int i = spe_module_num - 1; i >= 0; i--) {
    if (spe_modules[i]->module_type != SPE_USER_MODULE) continue;
    if (spe_modules[i]->exit_master && !spe_modules[i]->exit_master(&cycle)) {
      SPE_LOG_ERR("user module exit_master Error");
    }
  }
  // exit core module
  for (int i = spe_module_num - 1; i >= 0; i--) {
    if (spe_modules[i]->module_type != SPE_CORE_MODULE) continue;
    if (spe_modules[i]->exit_master && !spe_modules[i]->exit_master(&cycle)) {
      SPE_LOG_ERR("core module exit_master Error");
    }
  }
  // destroy speOpt
  spe_opt_destroy();
  return 0;
}
