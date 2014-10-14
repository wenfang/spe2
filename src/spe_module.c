#include "spe_module.h"
#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_signal.h"
#include "spe_server.h"
#include "spe_test.h"

speModule_t *speModules[] = {
  &speEpollModule,
  &speTaskModule,
  &speSignalModule,
  &speServerModule,
  &speTestModule,
  NULL,
};
