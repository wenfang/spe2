#ifndef __SPE_WORKER_H
#define __SPE_WORKER_H

#include <stdbool.h>
#include <sys/types.h>

extern int
spe_worker_fork(void);

extern int
spe_worker_reset(pid_t pid);

extern void
spe_worker_stop(void);

extern void 
spe_worker_process(void);

#endif
