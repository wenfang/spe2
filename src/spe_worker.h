#ifndef __SPE_WORKER_H
#define __SPE_WORKER_H

#include <stdbool.h>
#include <sys/types.h>

extern int
SpeWorkerFork();

extern int
SpeWorkerReset(pid_t pid);

extern void
SpeWorkerStop();

extern void 
SpeWorkerProcess();

#endif
