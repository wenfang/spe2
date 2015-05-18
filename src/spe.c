#include "spe.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static int spe_reap_worker;
static void reap_worker(int sig) {
  spe_reap_worker = 1;
}

static int spe_stop_worker;
static void stop_worker(int sig) {
  spe_stop_worker = 1;
}

static void
spe_master_process() {
  spe_save_pid(cycle.pidfile);
	spe_set_proc_title("spe: master");

  spe_signal_register(SIGPIPE, SIG_IGN);
  spe_signal_register(SIGHUP, SIG_IGN);
  spe_signal_register(SIGCHLD, reap_worker);
  spe_signal_register(SIGTERM, stop_worker);
  spe_signal_register(SIGINT, stop_worker);
  spe_signal_register(SIGUSR1, stop_worker);

  sigset_t set;
  sigemptyset(&set);
  for (;;) {
    sigsuspend(&set);
    spe_signal_process();
    if (spe_reap_worker) {
      for (;;) {
        int status;
        int pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) break;
        if (pid < 0) {
          SPE_LOG_ERR("waipid error");
          break;
        }
        if (spe_worker_reset(pid) < 0) {
          SPE_LOG_ERR("SpeProcessExit Error");
          continue;
        }
        // for new worker
        int res = spe_worker_fork();
        if (res < 0) {
          SPE_LOG_ERR("SpeProcessFork Error");
        } else if (res == 0) {
          spe_worker_process();
          return;
        }
        SPE_LOG_WARNING("SpeWorker Restart");
      }
      spe_reap_worker = 0;
    }
    if (spe_stop_worker) {
      spe_worker_stop();
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
      SPE_LOG_ERR("core module init_master");
      return 1;
    }
  }
  // init user module
  for (int i = 0; i < spe_module_num; i++) {
    if (spe_modules[i]->module_type != SPE_USER_MODULE) continue;
    if (spe_modules[i]->init_master && !spe_modules[i]->init_master(&cycle)) {
      SPE_LOG_ERR("user module init_master");
      return 1;
    }
  }
  // fork worker
  int res;
  for (int i=0; i<cycle.procs; i++) {
    res = spe_worker_fork();
    if (res <= 0) break;
  }
  if (res == 0) { // for worker
      spe_worker_process();
  } else if (res == 1) { // for master
      spe_master_process();
  } else {
    fprintf(stderr, "[ERROR] spe_worker_fork\n");
  }
  // exit user module
  for (int i = spe_module_num - 1; i >= 0; i--) {
    if (spe_modules[i]->module_type != SPE_USER_MODULE) continue;
    if (spe_modules[i]->exit_master && !spe_modules[i]->exit_master(&cycle)) {
      SPE_LOG_ERR("user module exit_master");
    }
  }
  // exit core module
  for (int i = spe_module_num - 1; i >= 0; i--) {
    if (spe_modules[i]->module_type != SPE_CORE_MODULE) continue;
    if (spe_modules[i]->exit_master && !spe_modules[i]->exit_master(&cycle)) {
      SPE_LOG_ERR("core module exit_master");
    }
  }
  // destroy speOpt
  spe_opt_destroy();
  return 0;
}
