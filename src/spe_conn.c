#include "spe_conn.h"
#include "spe_task.h"
#include "spe_sock.h"
#include "spe_util.h"
#include <errno.h>
#include <netdb.h>

#define BUF_SIZE  1024

#define SPE_CONN_READNONE   0
#define SPE_CONN_READ       1
#define SPE_CONN_READUNTIL  2
#define SPE_CONN_READBYTES  3

#define SPE_CONN_WRITENONE  0
#define SPE_CONN_WRITE      1

#define SPE_CONN_CONNECT    1

static spe_conn_t *conns;
static int conn_maxfd;

static void
connect_normal(void* arg) {
  spe_conn_t* conn = arg;
  // connect timeout
  if (conn->_read_expire_time && conn->_read_task.timeout) {
    conn->connect_timeout = 1;
    goto end_out;
  }
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->error = 1;
  if (err) conn->error = 1;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  if (conn->_read_expire_time) spe_task_dequeue(&conn->_read_task);
  conn->_read_type  = SPE_CONN_READNONE;
  conn->_write_type = SPE_CONN_WRITENONE;
  spe_task_schedule(&conn->post_read_task);
}

/*
===================================================================================================
spe_conn_connect
===================================================================================================
*/
bool
spe_conn_connect(spe_conn_t* conn, const char* addr, const char* port) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE && conn->_write_type == SPE_CONN_WRITENONE && 
      addr && port);
  // gen address hints
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  // get address info into servinfo
  struct addrinfo *servinfo;
  if (getaddrinfo(addr, port, &hints, &servinfo)) return false;
  // try the first address
  if (!servinfo) return false;
  if (connect(conn->_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      spe_task_set_handler(&conn->_read_task, SPE_HANDLER1(connect_normal, conn), 0);
      conn->connect_timeout = 0;
      conn->_read_type      = SPE_CONN_CONNECT;
      conn->_write_type     = SPE_CONN_CONNECT;
      spe_epoll_enable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE, &conn->_read_task);
      if (conn->_read_expire_time) {
        spe_task_schedule_timeout(&conn->_read_task, conn->_read_expire_time);
      }
      freeaddrinfo(servinfo);
      return true;
    }
    conn->error = 1;
  }
  // (sync): connect success or failed, call handler
  spe_task_schedule(&conn->post_read_task);
  freeaddrinfo(servinfo);
  return true;
}

static void
read_normal(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->_read_expire_time && conn->_read_task.timeout) {
    conn->read_timeout = 1;
    goto end_out;
  }
  // read data
  for (;;) {
    int res = spe_buf_read_fd_append(conn->_fd, BUF_SIZE, conn->_read_buffer);
    // read error
    if (res == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      SPE_LOG_ERR("conn error: %s", strerror(errno));
      conn->error = 1;
      break;
    }
    // peer close
    if (res == 0) {
      conn->closed = 1;
      break;
    }
    break;
  }
  // check read type
  if (conn->_read_type == SPE_CONN_READUNTIL) {
    int pos = spe_buf_search(conn->_read_buffer, conn->_delim);
    if (pos != -1) {
      unsigned len = pos + strlen(conn->_delim);
      spe_buf_append(conn->buffer, conn->_read_buffer->data, len);
      spe_buf_lconsume(conn->_read_buffer, len);
      goto end_out;
    }
  } else if (conn->_read_type == SPE_CONN_READBYTES) {
    if (conn->_rbytes <= conn->_read_buffer->len) {
      spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_rbytes);
      spe_buf_lconsume(conn->_read_buffer, conn->_rbytes);
      goto end_out;
    }
  } else if (conn->_read_type == SPE_CONN_READ) {
    if (conn->_read_buffer->len > 0) { 
      spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
      spe_buf_clean(conn->_read_buffer);
      goto end_out;
    }
  }
  // check error and close 
  if (conn->closed || conn->error) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ);
  if (conn->_read_expire_time) spe_task_dequeue(&conn->_read_task);
  conn->_read_type = SPE_CONN_READNONE;
  spe_task_schedule(&conn->post_read_task);
}

static bool
spe_conn_read_sync(spe_conn_t* conn) {
  int res = spe_buf_read_fd_append(conn->_fd, BUF_SIZE, conn->_read_buffer);
  if (res < 0 && errno != EINTR && errno != EAGAIN) conn->error = 1;
  if (res == 0) conn->closed = 1;
  if (conn->error || conn->closed) {
    spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
    spe_buf_clean(conn->_read_buffer);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  return false;
}

static void
spe_conn_read_async(spe_conn_t* conn, unsigned read_type) {
  spe_task_set_handler(&conn->_read_task, SPE_HANDLER1(read_normal, conn), 0);
  conn->read_timeout  = 0;
  conn->_read_type    = read_type;
  spe_epoll_enable(conn->_fd, SPE_EPOLL_READ, &conn->_read_task);
  if (conn->_read_expire_time) {
    spe_task_schedule_timeout(&conn->_read_task, conn->_read_expire_time);
  }
} 

/*
===================================================================================================
spe_conn_read_until
===================================================================================================
*/
bool
spe_conn_read_until(spe_conn_t* conn, char* delim) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE);
  if (!delim || conn->closed || conn->error) return false;
  // (sync):
  if (spe_conn_read_sync(conn)) return true;
  // check Buffer for delim
  int pos = spe_buf_search(conn->_read_buffer, delim);
  if (pos != -1) {
    unsigned len = pos + strlen(delim); 
    spe_buf_append(conn->buffer, conn->_read_buffer->data, len);
    spe_buf_lconsume(conn->_read_buffer, len);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  // (async):
  conn->_delim = delim;
  spe_conn_read_async(conn, SPE_CONN_READUNTIL);
  return true;
}

/*
===================================================================================================
spe_conn_readbytes
===================================================================================================
*/
bool
spe_conn_readbytes(spe_conn_t* conn, unsigned len) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE);
  if (len == 0 || conn->closed || conn->error ) return false;
  // (sync):
  if (spe_conn_read_sync(conn)) return true;
  // check buffer
  if (len <= conn->_read_buffer->len) {
    spe_buf_append(conn->buffer, conn->_read_buffer->data, len);
    spe_buf_lconsume(conn->_read_buffer, len);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  // (async):
  conn->_rbytes = len;
  spe_conn_read_async(conn, SPE_CONN_READBYTES);
  return true;
}

/*
===================================================================================================
spe_conn_read
===================================================================================================
*/
bool
spe_conn_read(spe_conn_t* conn) {
  ASSERT(conn && conn->_read_type == SPE_CONN_READNONE);
  if (conn->closed || conn->error) return false;
  // (sync):
  if (spe_conn_read_sync(conn)) return true;
  // check buffer
  if (conn->_read_buffer->len > 0) {
    spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
    spe_buf_clean(conn->_read_buffer);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  // (async):
  spe_conn_read_async(conn, SPE_CONN_READ);
  return true;
}

/*
===================================================================================================
write_normal
===================================================================================================
*/
static void
write_normal(void* arg) {
  spe_conn_t* conn = arg;
  // check timeout
  if (conn->_write_expire_time && conn->_write_task.timeout) {
    conn->write_timeout = 1;
    goto end_out;
  }
  // write date
  int res = write(conn->_fd, conn->_write_buffer->data, conn->_write_buffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->error = 1;
    }
    goto end_out;
  }
  spe_buf_lconsume(conn->_write_buffer, res);
  if (conn->_write_buffer->len == 0) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_WRITE);
  if (conn->_write_expire_time) spe_task_dequeue(&conn->_write_task);
  conn->_write_type = SPE_CONN_WRITENONE;
  spe_task_schedule(&conn->post_write_task);
}

/*
===================================================================================================
spe_conn_flush
===================================================================================================
*/
bool
spe_conn_flush(spe_conn_t* conn) {
  ASSERT(conn && conn->_write_type == SPE_CONN_WRITENONE);
  if (conn->closed || conn->error) return false;
  int res = write(conn->_fd, conn->_write_buffer->data, conn->_write_buffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->error = 1;
    }
    spe_task_schedule(&conn->post_write_task);
    return true;
  }
  spe_buf_lconsume(conn->_write_buffer, res);
  if (conn->_write_buffer->len == 0) {
    spe_task_schedule(&conn->post_write_task);
    return true;
  }
  spe_task_set_handler(&conn->_write_task, SPE_HANDLER1(write_normal, conn), 0);
  conn->write_timeout = 0;
  conn->_write_type   = SPE_CONN_WRITE;
  spe_epoll_enable(conn->_fd, SPE_EPOLL_WRITE, &conn->_write_task);
  if (conn->_write_expire_time) {
    spe_task_schedule_timeout(&conn->_write_task, conn->_write_expire_time);
  }
  return true;
}

/*
===================================================================================================
spe_conn_set_timeout
===================================================================================================
*/
void
spe_conn_set_timeout(spe_conn_t* conn, unsigned read_expire_time, unsigned write_expire_time) {
  ASSERT(conn);
  conn->_read_expire_time  = read_expire_time;
  conn->_write_expire_time = write_expire_time;
}

/*
===================================================================================================
init_connialize
===================================================================================================
*/
static bool
conn_initialize(spe_conn_t* conn, unsigned fd) {
  conn->_fd = fd;
  spe_task_init(&conn->_read_task);
  spe_task_init(&conn->_write_task);
  spe_task_init(&conn->post_read_task);
  spe_task_init(&conn->post_write_task);

  conn->buffer        = spe_buf_create();
  conn->_read_buffer  = spe_buf_create();
  conn->_write_buffer = spe_buf_create();
  if (!conn->buffer || !conn->_read_buffer || !conn->_write_buffer) {
    spe_buf_destroy(conn->buffer);
    spe_buf_destroy(conn->_read_buffer);
    spe_buf_destroy(conn->_write_buffer);
    return false;
  }
  return true;
}

/*
===================================================================================================
spe_conn_create
===================================================================================================
*/
spe_conn_t*
spe_conn_create(unsigned fd) {
  if (unlikely(fd >= conn_maxfd)) {
    SPE_LOG_ERR("conn overflow");
    return NULL;
  }
  spe_sock_set_block(fd, 0);
  spe_conn_t* conn = &conns[fd];
  if (conn->_fd == 0 && !conn_initialize(conn, fd)) {
    SPE_LOG_ERR("init_conn error");
    return NULL;
  }
  spe_buf_clean(conn->buffer);
  spe_buf_clean(conn->_read_buffer);
  spe_buf_clean(conn->_write_buffer);
  // init conn status
  conn->_read_expire_time   = 0;
  conn->_write_expire_time  = 0;
  conn->_read_type          = SPE_CONN_READNONE;
  conn->_write_type         = SPE_CONN_WRITENONE;
  conn->closed              = 0;
  conn->error               = 0;
  return conn;
}

/*
===================================================================================================
spe_conn_destroy
===================================================================================================
*/
void
spe_conn_destroy(spe_conn_t* conn) {
  ASSERT(conn);
  spe_task_dequeue(&conn->_read_task);
  spe_task_dequeue(&conn->_write_task);
  spe_task_dequeue(&conn->post_read_task);
  spe_task_dequeue(&conn->post_write_task);
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  spe_sock_close(conn->_fd);
}

static bool
init_conn(spe_cycle_t *cycle) {
  conns = calloc(1, sizeof(spe_conn_t)*cycle->maxfd);
  if (!conns) return false;
  conn_maxfd = cycle->maxfd;
  return true;
}

static bool
exit_conn(spe_cycle_t *cycle) {
  free(conns);
  return true;
}

spe_module_t spe_conn_module = {
  "spe_conn",
  0,
  SPE_CORE_MODULE,
  NULL,
  init_conn,
  exit_conn,
  NULL,
};
