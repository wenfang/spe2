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
serverAccept(spe_server_t* server) {
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
  spe_task_schedule(&conn->PostReadTask);
}

/*
===================================================================================================
SpeServerPreLoop
===================================================================================================
*/
void
SpeServerPreLoop() {
  spe_server_t *server = NULL;
  list_for_each_entry(server, &serverHead, serverNode) {
    if (!pthread_mutex_trylock(server->acceptMutex)) {
      if (server->acceptMutexHold) continue;
      server->acceptMutexHold = 1;
      SpeEpollEnable(server->sfd, SPE_EPOLL_LISTEN, &server->listen_task);
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
  spe_server_t *server = NULL;
  list_for_each_entry(server, &serverHead, serverNode) {
    if (server->acceptMutexHold) pthread_mutex_unlock(server->acceptMutex);
  }
}

/*
===================================================================================================
spe_server_register
===================================================================================================
*/
spe_server_t*
spe_server_register(const char* addr, int port, spe_server_handler handler, void* arg) {
  // create server fd
  int sfd = SpeSockTcpServer(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("SpeSockTcpServer error %s:%d", addr, port);
    return NULL;
  }
  SpeSockSetBlock(sfd, 0);
  // create server
  spe_server_t *server = calloc(1, sizeof(spe_server_t));
  if (!server) {
    SPE_LOG_ERR("spe_server_t calloc error");
    SpeSockClose(sfd);
    return NULL;
  }
  server->sfd     = sfd;
  server->handler = handler;
  server->arg     = arg;
  spe_task_init(&server->listen_task, SPE_TASK_FAST);
  server->listen_task.Handler = SPE_HANDLER1(serverAccept, server);
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
spe_server_unregister
===================================================================================================
*/
bool
spe_server_unregister(spe_server_t* server) {
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
exit_server
===================================================================================================
*/
static bool
exit_server() {
  spe_server_t *server, *tmp;
  list_for_each_entry_safe(server, tmp, &serverHead, serverNode) {
    if (server->acceptMutex) SpeShmMutexDestroy(server->acceptMutex);
    SpeSockClose(server->sfd);
    free(server);
  }
  return true;
}

spe_module_t spe_server_module = {
  "spe_server",
  0,
  SPE_CORE_MODULE,
  NULL,
  NULL,
  NULL,
  exit_server,
};
