#ifndef __SPE_SHM_H
#define __SPE_SHM_H

#include <pthread.h>

struct speShm_s {
  void*     addr;
  unsigned  size;
};
typedef struct speShm_s speShm_t;

extern speShm_t*
SpeShmCreate(unsigned size);

extern void
SpeShmDestroy(speShm_t* shm);

extern pthread_mutex_t*
SpeShmMutexCreate();

extern void
SpeShmMutexDestroy(pthread_mutex_t* shmux);

#endif
