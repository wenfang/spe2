#ifndef __SPE_MODULE_H
#define __SPE_MODULE_H

#include "spe_cycle.h"

#define SPE_CORE_MODULE 1
#define SPE_USER_MODULE 2

typedef struct spe_module_s {
  const char* name;
  int         index;
  int         module_type;

  bool  (*init_master)(spe_cycle_t*);
  bool  (*init_worker)(spe_cycle_t*);
  bool  (*exit_worker)(spe_cycle_t*);
  bool  (*exit_master)(spe_cycle_t*);
} spe_module_t;

extern int spe_module_num;
extern spe_module_t *spe_modules[];

extern bool
spe_module_init();

#endif
