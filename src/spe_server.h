#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_conn.h"
#include "spe_module.h"
#include <stdbool.h>

typedef void (*spe_server_handler)(speConn_t*, void*);

typedef struct spe_server_s {
  unsigned            sfd;
  spe_server_handler  handler;
  void                *arg;
  spe_task_t          listen_task;
  pthread_mutex_t     *acceptMutex;
  unsigned            acceptMutexHold;
  struct list_head    serverNode;
} spe_server_t __attribute__((aligned(sizeof(long))));

extern void
SpeServerPreLoop();

extern void
SpeServerPostLoop();

extern spe_server_t*
spe_server_register(const char* addr, int port, spe_server_handler handler, void* arg);

extern bool
spe_server_unregister(spe_server_t* server);

extern spe_module_t spe_server_module;

#endif
