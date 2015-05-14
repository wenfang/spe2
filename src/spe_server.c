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
static LIST_HEAD(server_head);

static void
server_accept(spe_server_t* server) {
  int cfd = spe_sock_accept(server->_sfd);
  if (cfd <= 0) return;
  if (!server->_handler) {
    SPE_LOG_ERR("Handler Not Set");
    spe_sock_close(cfd);
    return;
  }
  spe_conn_t* conn = spe_conn_create(cfd);
  if (!conn) {
    SPE_LOG_ERR("spe_conn_create Error");
    spe_sock_close(cfd);
    return;
  }
  spe_task_set_handler(&conn->post_read_task, SPE_HANDLER2(server->_handler, conn, server->_arg), 0);
  spe_task_schedule(&conn->post_read_task);
}

/*
===================================================================================================
spe_server_preloop
===================================================================================================
*/
void
spe_server_preloop(void) {
  spe_server_t *server = NULL;
  list_for_each_entry(server, &server_head, _server_node) {
    if (!pthread_mutex_trylock(server->_accept_mutex)) {
      if (server->_accept_mutex_hold) continue;
      server->_accept_mutex_hold = 1;
      spe_epoll_enable(server->_sfd, SPE_EPOLL_LISTEN, &server->_listen_task);
      continue;
    }
    if (server->_accept_mutex_hold) {
      spe_epoll_disable(server->_sfd, SPE_EPOLL_LISTEN);
      server->_accept_mutex_hold = 0;
    }
  }
}

/*
===================================================================================================
spe_server_postloop
===================================================================================================
*/
void
spe_server_postloop(void) {
  spe_server_t *server = NULL;
  list_for_each_entry(server, &server_head, _server_node) {
    if (server->_accept_mutex_hold) pthread_mutex_unlock(server->_accept_mutex);
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
  int sfd = spe_sock_tcp_server(addr, port);
  if (sfd < 0) {
    SPE_LOG_ERR("spe_sock_tcp_server error %s:%d", addr, port);
    return NULL;
  }
  spe_sock_set_block(sfd, 0);
  // create server
  spe_server_t *server = calloc(1, sizeof(spe_server_t));
  if (!server) {
    SPE_LOG_ERR("spe_server_t calloc error");
    spe_sock_close(sfd);
    return NULL;
  }
  server->_sfd      = sfd;
  server->_handler  = handler;
  server->_arg      = arg;
  spe_task_init(&server->_listen_task);
  spe_task_set_handler(&server->_listen_task, SPE_HANDLER1(server_accept, server), 1);
  server->_accept_mutex = spe_shm_mutex_create();
  if (!server->_accept_mutex) {
    SPE_LOG_ERR("SpeShmuxCreate error");
    spe_sock_close(sfd);
    free(server);
    return NULL;
  }
  INIT_LIST_HEAD(&server->_server_node);
  list_add_tail(&server->_server_node, &server_head);
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
  if (server->_accept_mutex_hold) {
    spe_epoll_disable(server->_sfd, SPE_EPOLL_LISTEN);
  }
  if (server->_accept_mutex) spe_shm_mutex_destroy(server->_accept_mutex);
  spe_sock_close(server->_sfd);
  list_del(&server->_server_node);
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
  list_for_each_entry_safe(server, tmp, &server_head, _server_node) {
    if (server->_accept_mutex) spe_shm_mutex_destroy(server->_accept_mutex);
    spe_sock_close(server->_sfd);
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
