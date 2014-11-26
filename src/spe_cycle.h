#ifndef __SPE_CYCLE_H
#define __SPE_CYCLE_H

#include <stdbool.h>

#define SPE_MODULE_MAX  128

typedef struct {
  int   daemon;
  int   procs;
  int   maxfd;
  void  *ctx[SPE_MODULE_MAX];
} speCycle_t;

bool SpeCycleInit();

extern speCycle_t cycle;

#endif
