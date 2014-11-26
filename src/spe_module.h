#ifndef __SPE_MODULE_H
#define __SPE_MODULE_H

#include "spe_cycle.h"

#define SPE_CORE_MODULE 1
#define SPE_USER_MODULE 2

typedef struct {
  const char* name;
  int         index;
  int         moduleType;

  void  (*initMaster)(speCycle_t*);
  void  (*initWorker)(speCycle_t*);
  void  (*exitWorker)(speCycle_t*);
  void  (*exitMaster)(speCycle_t*);
} speModule_t;

extern speModule_t *speModules[];

#endif
