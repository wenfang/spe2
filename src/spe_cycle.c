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
SpeCycleInit(int speModuleNum) {
  cycle.daemon = SpeOptInt("global", "Daemon", 0);
  cycle.procs = SpeOptInt("global", "Procs", 1);
  if (cycle.procs <= 0) {
    return false;
  }
  cycle.ctx = calloc(speModuleNum, sizeof(void*));
  if (!cycle.ctx) return false;
  return true;
}

/*
===================================================================================================
SpeCycleDestroy
===================================================================================================
*/
void 
SpeCycleDestroy() {
  free(cycle.ctx);
}
