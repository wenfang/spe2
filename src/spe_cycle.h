#ifndef __SPE_CYCLE_H
#define __SPE_CYCLE_H

#include <stdbool.h>

struct speCycle_s {
  char  *confFile; 
  int   daemon;
  int   procs;
  void  **ctx;
};
typedef struct speCycle_s speCycle_t;

bool SpeCycleInit(int speModuleNum);
void SpeCycleDestroy();

extern speCycle_t cycle;

#endif
