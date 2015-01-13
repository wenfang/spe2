#include "spe_server.h"
#include "spe_opt.h"
#include "spe_task.h"
#include "spe_shm.h"
#include "spe_epoll.h"
#include "spe_sock.h"
#include "spe_util.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct speServer_s {
  unsigned            sfd;
  unsigned            stop;
  SpeServerHandler    handler;
  void                *arg;
  speTask_t           listenTask;
  pthread_mutex_t     *acceptMutex;
  unsigned            acceptMutexHold;
  struct speServer_s  *next;
} __attribute__((aligned(sizeof(long))));
typedef struct speServer_s speServer_t;

// global server list
static speServer_t *gServer;

static void
serverAccept(speServer_t* server) {
  int cfd = SpeSockAccept(server->sfd);
  if (cfd <= 0) return;
  if (!server->handler) {
    SPE_LOG_ERR("server no handler set");
    SpeSockClose(cfd);
    return;
  }
  speConn_t* conn = SpeConnCreate(cfd);
  if (!conn) {
    SPE_LOG_ERR("SpeConnCreate Error");
    SpeSockClose(cfd);
    return;
  }
  conn->ReadCallback.Handler = SPE_HANDLER2(server->handler, conn, server->arg);
  SpeTaskEnqueue(&conn->ReadCallback);
}

/*
===================================================================================================
SpeServerPreLoop
===================================================================================================
*/
void
SpeServerPreLoop() {
  for (speServer_t* server = gServer; server != NULL; server = server->next) {
    if (!server->stop && !pthread_mutex_trylock(server->acceptMutex)) {
      if (server->acceptMutexHold) continue;
      server->acceptMutexHold = 1;
      SpeEpollEnable(server->sfd, SPE_EPOLL_LISTEN, &server->listenTask);
    } else if (server->acceptMutexHold) {
      SpeEpollDisable(server->sfd, SPE_EPOLL_LISTEN);
      server->acceptMutexHold = 0;
    }
  }
}

/*
===================================================================================================
SpeServerPostLoop
===================================================================================================
*/
void
SpeServerPostLoop() {
  for (speServer_t* server = gServer; server != NULL; server = server->next) {
    if (server->acceptMutexHold) pthread_mutex_unlock(server->acceptMutex);
  }
}

/*
===================================================================================================
SpeServerRegister
===================================================================================================
*/
bool
SpeServerRegister(const char* addr, int port, SpeServerHandler handler, void* arg) {
  // create server fd
  int sfd = SpeSockTcpServer(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("SpeSockTcpServer error %s:%d", addr, port);
    return false;
  }
  SpeSockSetBlock(sfd, 0);
  // create gServer
  speServer_t *server = calloc(1, sizeof(speServer_t));
  if (!server) {
    SPE_LOG_ERR("speServer_t alloc error");
    SpeSockClose(sfd);
    return false;
  }
  server->sfd     = sfd;
  server->stop    = 0;
  server->handler = handler;
  server->arg     = arg;
  SpeTaskInit(&server->listenTask, SPE_TASK_FAST);
  server->listenTask.Handler = SPE_HANDLER1(serverAccept, server);
  server->acceptMutex = SpeShmMutexCreate();
  if (!server->acceptMutex) {
    SPE_LOG_ERR("SpeShmuxCreate error");
    SpeSockClose(sfd);
    free(server);
    return false;
  }
  server->next  = gServer;
  gServer       = server;
  return true;
}

/*
===================================================================================================
SpeServerStop
===================================================================================================
*/
void
SpeServerStop() {
  for (speServer_t* server = gServer; server != NULL; server = server->next) {
    server->stop = 1;
  }
}

/*
===================================================================================================
serverDeinit
===================================================================================================
*/
static bool
serverDeinit() {
  for (speServer_t* server = gServer; server != NULL; server = server->next) {
    if (server->acceptMutex) SpeShmMutexDestroy(server->acceptMutex);
    SpeSockClose(server->sfd);
    free(server);
  }
  return true;
}

speModule_t speServerModule = {
  "speServer",
  0,
  SPE_CORE_MODULE,
  NULL,
  NULL,
  NULL,
  serverDeinit,
};
