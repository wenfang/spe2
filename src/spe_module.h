#ifndef __SPE_MODULE_H
#define __SPE_MODULE_H

#include "spe_cycle.h"

#define SPE_CORE_MODULE 1
#define SPE_USER_MODULE 2

typedef struct {
  const char* name;
  int         index;
  int         moduleType;

  bool  (*initMaster)(speCycle_t*);
  bool  (*initWorker)(speCycle_t*);
  bool  (*exitWorker)(speCycle_t*);
  bool  (*exitMaster)(speCycle_t*);
} speModule_t;

extern speModule_t *speModules[];

#endif
