#include "spe_cycle.h"
#include "spe_opt.h"
#include <stdlib.h>

spe_cycle_t cycle;

/*
===================================================================================================
spe_cycle_init
===================================================================================================
*/
bool 
spe_cycle_init() {
  cycle.daemon = spe_opt_int("global", "daemon", 0);
  if ((cycle.procs = spe_opt_int("global", "procs", 1)) <= 0) return false;
  if ((cycle.maxfd = spe_opt_int("global", "maxfd", 1000000)) <= 0) return false;
  cycle.pidfile = spe_opt_string("global", "pidfile", "/tmp/spe.pid");
  return true;
}
