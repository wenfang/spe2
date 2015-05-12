#include "spe_module.h"
//#include "spe_epoll.h"
#include "spe_task.h"
//#include "spe_signal.h"
//#include "spe_server.h"
//#include "spe_lua.h"
//#include "spe_test.h"

spe_module_t *spe_modules[] = {
 // &speEpollModule,
 // &speConnModule,
  &spe_task_module,
 // &speSignalModule,
 // &speServerModule,
 // &speLuaModule,
 // &speTestModule,
  NULL,
};

bool
spe_module_init() {
  int i;
  for (i = 0; spe_modules[i] != NULL; i++) {
    if (i > SPE_MODULE_MAX) {
      return false;
    }
    spe_modules[i]->index = i;
  }
  return true;
}
