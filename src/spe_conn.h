#ifndef __SPE_CONN_H
#define __SPE_CONN_H

#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_buf.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <string.h>

typedef struct {
  int       fd;
  speTask_t readTask;
  speTask_t writeTask;
  speTask_t ReadCallback;
  speTask_t WriteCallback;
  unsigned  readExpireTime;
  unsigned  writeExpireTime;
  speBuf_t* ReadBuffer;
  speBuf_t* writeBuffer;
  unsigned  RLen;
  char*     delim;
  unsigned  rbytes;
  unsigned  readType:2;
  unsigned  writeType:1;
  unsigned  ConnectTimeout:1;
  unsigned  ReadTimeout:1;
  unsigned  WriteTimeout:1;
  unsigned  Closed:1;
  unsigned  Error:1;
} SpeConn_t __attribute__((aligned(sizeof(long))));

extern bool
SpeConnConnect(SpeConn_t* conn, const char* addr, const char* port);

extern bool
SpeConnRead(SpeConn_t* conn);

extern bool
SpeConnReadbytes(SpeConn_t* conn, unsigned len);

extern bool
SpeConnReaduntil(SpeConn_t* conn, char* delim);

static inline bool
SpeConnWrite(SpeConn_t* conn, char* buf, unsigned len) {
  ASSERT(conn && buf && len);
  if (conn->Closed || conn->Error) return false;
  return SpeBufCat(conn->writeBuffer, buf, len);
}

extern bool
SpeConnFlush(SpeConn_t* conn);

extern bool
SpeConnSetTimeout(SpeConn_t* conn, unsigned readExpireTime, unsigned writeExpireTime);

extern SpeConn_t*
SpeConnCreate(unsigned fd);

extern void
SpeConnDestroy(SpeConn_t* conn);

#endif
