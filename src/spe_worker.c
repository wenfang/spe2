#include "spe_process.h"
#include "spe_worker.h"
#include "spe_module.h"
#include "spe_server.h"
#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_signal.h"
#include <signal.h>

bool speWorkerStop;

static void workerCtrlHandler() {
}

void
SpeWorkerProcess() {
  SpeSignalRegister(SIGPIPE, SIG_IGN);
  SpeSignalRegister(SIGHUP, SIG_IGN);
  SpeSignalRegister(SIGCHLD, SIG_IGN);
  SpeSignalRegister(SIGUSR1, SIG_IGN);

  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->initWorker) speModules[i]->initWorker(&cycle);
  }
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->initWorker) speModules[i]->initWorker(&cycle);
  }
  // enable control task
  SpeProcessEnableControl(SPE_HANDLER0(workerCtrlHandler));
  // event loop
  unsigned timeout = 300;
  while (!speWorkerStop) {
    if (speTaskNum) timeout = 0;
    SpeServerPreLoop();
    SpeEpollProcess(timeout);
    SpeServerPostLoop();
    SpeTaskProcess();
    SpeSignalProcess();
  }
  // disable control task
  SpeProcessDisableControl();

  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->exitWorker) speModules[i]->exitWorker(&cycle);
  }
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->exitWorker) speModules[i]->exitWorker(&cycle);
  }
}
