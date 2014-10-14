#include "spe_shm.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdlib.h>
#include <sys/mman.h>

/*
===================================================================================================
SpeShmCreate
===================================================================================================
*/
speShm_t*
SpeShmCreate(unsigned size) {
  speShm_t* shm = calloc(1, sizeof(speShm_t));
  if (!shm) {
    SPE_LOG_ERR("spe_shm alloc calloc error");
    return NULL;
  }
  shm->addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (!shm->addr) {
    SPE_LOG_ERR("spe shm alloc mmap error");
    free(shm);
    return NULL; 
  } 
  shm->size = size;
  return shm;
}

/*
===================================================================================================
SpeShmDestroy
===================================================================================================
*/
void
SpeShmDestroy(speShm_t* shm) {
  ASSERT(shm);
  if (munmap(shm->addr, shm->size) == -1) {
    SPE_LOG_ERR("SpeShmFree error");
  }
  free(shm);
}

/*
===================================================================================================
SpeShmMutexCreate
===================================================================================================
*/
pthread_mutex_t*
SpeShmMutexCreate() {
  pthread_mutex_t* shmux = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE,
      MAP_ANON|MAP_SHARED, -1, 0);
  if (!shmux) {
    SPE_LOG_ERR("SpeShmMutexCreate mmap error");
    return NULL;
  }
  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(shmux, &mattr);
  return shmux;
}

/*
===================================================================================================
SpeShmMutexDestroy
===================================================================================================
*/
void
SpeShmMutexDestroy(pthread_mutex_t* shmux) {
  ASSERT(shmux);
  pthread_mutex_destroy(shmux);
  if (munmap(shmux, sizeof(pthread_mutex_t) == -1)) {
    SPE_LOG_ERR("SpeShmuxDestroy error");
  }
}
