#include "spe_cycle.h"
#include "spe_opt.h"
#include <stdlib.h>

speCycle_t cycle;

/*
===================================================================================================
SpeCycleInit
===================================================================================================
*/
bool 
SpeCycleInit() {
  cycle.daemon = SpeOptInt("global", "daemon", 0);
  if ((cycle.procs = SpeOptInt("global", "procs", 1)) <= 0) return false;
  if ((cycle.maxfd = SpeOptInt("global", "maxfd", 1000000)) <= 0) return false;
  cycle.pidfile = SpeOptString("global", "pidfile", "/tmp/spe.pid");
  return true;
}
