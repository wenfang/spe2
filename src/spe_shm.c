#include "spe_shm.h"
#include "spe_worker.h"
#include "spe_log.h"
#include "spe_util.h"
#include <stdlib.h>
#include <sys/mman.h>

/*
===================================================================================================
spe_shm_create
===================================================================================================
*/
spe_shm_t*
spe_shm_create(unsigned size) {
  spe_shm_t* shm = calloc(1, sizeof(spe_shm_t));
  if (!shm) {
    SPE_LOG_ERR("spe shm calloc error");
    return NULL;
  }
  shm->_addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (!shm->_addr) {
    SPE_LOG_ERR("spe shm alloc mmap error");
    free(shm);
    return NULL; 
  } 
  shm->_size = size;
  return shm;
}

/*
===================================================================================================
spe_shm_destroy
===================================================================================================
*/
void
spe_shm_destroy(spe_shm_t* shm) {
  ASSERT(shm);
  if (munmap(shm->_addr, shm->_size) == -1) {
    SPE_LOG_ERR("spe shm munmap error");
  }
  free(shm);
}

/*
===================================================================================================
spe_shm_mutex_create
===================================================================================================
*/
pthread_mutex_t*
spe_shm_mutex_create(void) {
  pthread_mutex_t* shmux = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE,
      MAP_ANON|MAP_SHARED, -1, 0);
  if (!shmux) {
    SPE_LOG_ERR("spe_shm_mutex_create mmap error");
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
spe_shm_mutex_destroy
===================================================================================================
*/
void
spe_shm_mutex_destroy(pthread_mutex_t* shmux) {
  ASSERT(shmux);
  pthread_mutex_destroy(shmux);
  if (munmap(shmux, sizeof(pthread_mutex_t)) == -1) {
    SPE_LOG_ERR("SpeShmuxDestroy error");
  }
}
