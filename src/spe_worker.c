#include "spe_worker.h"
#include "spe_module.h"
#include "spe_server.h"
#include "spe_task.h"
#include "spe_epoll.h"
#include "spe_signal.h"
#include "google/profiler.h"

bool speWorkerStop;

/*
static void workerControl(int *fd) {
  int cfd = *fd;
  int msg = 0;
  read(cfd, &msg, sizeof(msg));
}
*/

void
SpeWorkerProcess() {
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->initWorker) {
      speModules[i]->initWorker(&cycle);
    }
  }
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->initWorker) {
      speModules[i]->initWorker(&cycle);
    }
  }

  unsigned timeout = 300;
  while (!speWorkerStop) {
    if (speTaskNum) timeout = 0;
    SpeServerPreLoop();
    SpeEpollProcess(timeout);
    SpeServerPostLoop();
    SpeTaskProcess();
    SpeSignalProcess();
  }

  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_USER_MODULE) continue;
    if (speModules[i]->exitWorker) {
      speModules[i]->exitWorker(&cycle);
    }
  }
  for (int i = 0; speModules[i] != NULL; i++) {
    if (speModules[i]->moduleType != SPE_CORE_MODULE) continue;
    if (speModules[i]->exitWorker) {
      speModules[i]->exitWorker(&cycle);
    }
  }
}
