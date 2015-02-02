#include "spe_module.h"
#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_signal.h"
#include "spe_server.h"
#include "spe_lua.h"
#include "spe_test.h"

speModule_t *speModules[] = {
  &speEpollModule,
  &speConnModule,
  &speTaskModule,
  &speSignalModule,
  &speServerModule,
  &speLuaModule,
  &speTestModule,
  NULL,
};

int speModuleNum;

bool
speModuleInit() {
  int i;
  for (i = 0; speModules[i] != NULL; i++) {
    if (i > SPE_MODULE_MAX) {
      return false;
    }
    speModules[i]->index = i;
  }
  speModuleNum = i;
  return true;
}
