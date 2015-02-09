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

// global server list
static LIST_HEAD(serverHead);

static void
serverAccept(speServer_t* server) {
  int cfd = SpeSockAccept(server->sfd);
  if (cfd <= 0) return;
  if (!server->handler) {
    SPE_LOG_ERR("Handler Not Set");
    SpeSockClose(cfd);
    return;
  }
  speConn_t* conn = SpeConnCreate(cfd);
  if (!conn) {
    SPE_LOG_ERR("SpeConnCreate Error");
    SpeSockClose(cfd);
    return;
  }
  conn->PostReadTask.Handler = SPE_HANDLER2(server->handler, conn, server->arg);
  SpeTaskEnqueue(&conn->PostReadTask);
}

/*
===================================================================================================
SpeServerPreLoop
===================================================================================================
*/
void
SpeServerPreLoop() {
  speServer_t *server = NULL;
  list_for_each_entry(server, &serverHead, serverNode) {
    if (!pthread_mutex_trylock(server->acceptMutex)) {
      if (server->acceptMutexHold) continue;
      server->acceptMutexHold = 1;
      SpeEpollEnable(server->sfd, SPE_EPOLL_LISTEN, &server->listenTask);
      continue;
    }
    if (server->acceptMutexHold) {
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
  speServer_t *server = NULL;
  list_for_each_entry(server, &serverHead, serverNode) {
    if (server->acceptMutexHold) pthread_mutex_unlock(server->acceptMutex);
  }
}

/*
===================================================================================================
SpeServerRegister
===================================================================================================
*/
speServer_t*
SpeServerRegister(const char* addr, int port, SpeServerHandler handler, void* arg) {
  // create server fd
  int sfd = SpeSockTcpServer(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("SpeSockTcpServer error %s:%d", addr, port);
    return NULL;
  }
  SpeSockSetBlock(sfd, 0);
  // create server
  speServer_t *server = calloc(1, sizeof(speServer_t));
  if (!server) {
    SPE_LOG_ERR("speServer_t calloc error");
    SpeSockClose(sfd);
    return NULL;
  }
  server->sfd     = sfd;
  server->handler = handler;
  server->arg     = arg;
  SpeTaskInit(&server->listenTask, SPE_TASK_FAST);
  server->listenTask.Handler = SPE_HANDLER1(serverAccept, server);
  server->acceptMutex = SpeShmMutexCreate();
  if (!server->acceptMutex) {
    SPE_LOG_ERR("SpeShmuxCreate error");
    SpeSockClose(sfd);
    free(server);
    return NULL;
  }
  INIT_LIST_HEAD(&server->serverNode);
  list_add_tail(&server->serverNode, &serverHead);
  return server;
}

/*
===================================================================================================
SpeServerUnregister
===================================================================================================
*/
bool
SpeServerUnregister(speServer_t* server) {
  ASSERT(server);
  if (server->acceptMutexHold) {
    SpeEpollDisable(server->sfd, SPE_EPOLL_LISTEN);
  }
  if (server->acceptMutex) SpeShmMutexDestroy(server->acceptMutex);
  SpeSockClose(server->sfd);
  list_del(&server->serverNode);
  free(server);
  return true;
}

/*
===================================================================================================
serverDeinit
===================================================================================================
*/
static bool
serverDeinit() {
  speServer_t *server, *tmp;
  list_for_each_entry_safe(server, tmp, &serverHead, serverNode) {
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
