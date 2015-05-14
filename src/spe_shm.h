#ifndef __SPE_SHM_H
#define __SPE_SHM_H

#include <pthread.h>

typedef struct spe_shm_s {
  void*     _addr;
  unsigned  _size;
} spe_shm_t;

extern spe_shm_t*
spe_shm_create(unsigned size);

extern void
spe_shm_destroy(spe_shm_t* shm);

extern pthread_mutex_t*
spe_shm_mutex_create(void);

extern void
spe_shm_mutex_destroy(pthread_mutex_t* shmux);

#endif
