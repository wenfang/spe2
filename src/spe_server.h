#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_conn.h"
#include "spe_module.h"
#include <stdbool.h>

typedef void (*SpeServerHandler)(speConn_t*, void*);

typedef struct {
  unsigned            sfd;
  SpeServerHandler    handler;
  void                *arg;
  speTask_t           listenTask;
  pthread_mutex_t     *acceptMutex;
  unsigned            acceptMutexHold;
  struct list_head    serverNode;
} speServer_t __attribute__((aligned(sizeof(long))));

extern void
SpeServerPreLoop();

extern void
SpeServerPostLoop();

extern speServer_t*
SpeServerRegister(const char* addr, int port, SpeServerHandler handler, void* arg);

extern bool
SpeServerUnregister(speServer_t* server);

extern speModule_t speServerModule;

#endif
