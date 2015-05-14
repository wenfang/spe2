#ifndef __SPE_CONN_H
#define __SPE_CONN_H

#include "spe_module.h"
#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_buf.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <string.h>

typedef struct {
  int         _fd;
  spe_task_t  _read_task;
  spe_task_t  _write_task;
  spe_task_t  post_read_task;
  spe_task_t  post_write_task;
  unsigned    _read_expire_time;
  unsigned    _write_expire_time;
  spe_buf_t*  buffer;
  spe_buf_t*  _read_buffer;
  spe_buf_t*  _write_buffer;

  char*     _delim;
  unsigned  _rbytes;
  unsigned  _read_type:2;
  unsigned  _write_type:1;
  unsigned  connect_timeout:1;
  unsigned  read_timeout:1;
  unsigned  write_timeout:1;
  unsigned  closed:1;
  unsigned  error:1;
} spe_conn_t __attribute__((aligned(sizeof(long))));

extern bool
spe_conn_connect(spe_conn_t* conn, const char* addr, const char* port);

extern bool
spe_conn_read(spe_conn_t* conn);

extern bool
spe_conn_readbytes(spe_conn_t* conn, unsigned len);

extern bool
spe_conn_read_until(spe_conn_t* conn, char* delim);

static inline bool
spe_conn_write(spe_conn_t* conn, char* buf, unsigned len) {
  ASSERT(conn && buf && len);
  if (conn->closed || conn->error) return false;
  return spe_buf_append(conn->_write_buffer, buf, len);
}

extern bool
spe_conn_flush(spe_conn_t* conn);

extern void
spe_conn_set_timeout(spe_conn_t* conn, unsigned read_expire_time, unsigned write_expire_time);

extern spe_conn_t*
spe_conn_create(unsigned fd);

extern void
spe_conn_destroy(spe_conn_t* conn);

extern spe_module_t speConnModule;

#endif
