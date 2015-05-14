#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_conn.h"
#include "spe_module.h"
#include <stdbool.h>

typedef void (*spe_server_handler)(spe_conn_t*, void*);

typedef struct spe_server_s {
  unsigned            _sfd;
  spe_server_handler  _handler;
  void*               _arg;
  spe_task_t          _listen_task;
  pthread_mutex_t*    _accept_mutex;
  unsigned            _accept_mutex_hold;
  struct list_head    _server_node;
} spe_server_t __attribute__((aligned(sizeof(long))));

extern void
spe_server_preloop(void);

extern void
spe_server_postloop(void);

extern spe_server_t*
spe_server_register(const char* addr, int port, spe_server_handler handler, void* arg);

extern bool
spe_server_unregister(spe_server_t* server);

extern spe_module_t spe_server_module;

#endif
