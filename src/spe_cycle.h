#ifndef __SPE_CYCLE_H
#define __SPE_CYCLE_H

#include <stdbool.h>

typedef struct {
  char  *confFile; 
  int   daemon;
  int   procs;
  void  **ctx;
} speCycle_t;

bool SpeCycleInit(int speModuleNum);
void SpeCycleDestroy();

extern speCycle_t cycle;

#endif
