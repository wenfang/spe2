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
  cycle.procs = SpeOptInt("global", "procs", 1);
  if (cycle.procs <= 0) return false;
  cycle.maxfd = SpeOptInt("global", "maxfd", 1000000);
  if (cycle.maxfd <= 0) return false;
  return true;
}
