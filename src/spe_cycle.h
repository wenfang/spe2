#ifndef __SPE_CYCLE_H
#define __SPE_CYCLE_H

#include <stdbool.h>

#define SPE_MODULE_MAX  128

typedef struct spe_cycle_s {
  int         daemon;
  int         procs;
  int         maxfd;
  const char  *pidfile;
  void        *ctx[SPE_MODULE_MAX];
} spe_cycle_t;

extern
bool spe_cycle_init();

extern spe_cycle_t cycle;

#endif
