#ifndef __SPE_CONN_H
#define __SPE_CONN_H

#include "spe_epoll.h"
#include "spe_task.h"
#include "spe_string.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <string.h>

struct SpeConn_s {
  int           fd;
  speTask_t     readTask;
  speTask_t     writeTask;
  speTask_t     ReadCallback;
  speTask_t     WriteCallback;
  unsigned      readExpireTime;
  unsigned      writeExpireTime;
  SpeString_t*  readBuffer;
  SpeString_t*  writeBuffer;
  SpeString_t*  Buffer;
  char*         delim;
  unsigned      rbytes;
  unsigned      readType:2;
  unsigned      writeType:1;
  unsigned      ConnectTimeout:1;
  unsigned      ReadTimeout:1;
  unsigned      WriteTimeout:1;
  unsigned      Closed:1;
  unsigned      Error:1;
} __attribute__((aligned(sizeof(long))));
typedef struct SpeConn_s SpeConn_t;

extern bool
SpeConnConnect(SpeConn_t* conn, const char* addr, const char* port);

extern bool
SpeConnRead(SpeConn_t* conn);

extern bool
SpeConnReadbytes(SpeConn_t* conn, unsigned len);

extern bool
SpeConnReaduntil(SpeConn_t* conn, char* delim);

static inline bool
SpeConnWrite(SpeConn_t* conn, SpeString_t* buf) {
  ASSERT(conn && buf);
  if (conn->Closed || conn->Error) return false;
  return SpeStringCatb(conn->writeBuffer, buf->data, buf->len);
}

static inline bool
SpeConnWriteb(SpeConn_t* conn, char* buf, unsigned len) {
  ASSERT(conn && buf && len);
  if (conn->Closed || conn->Error) return false;
  return SpeStringCatb(conn->writeBuffer, buf, len);
}

static inline bool
SpeConnWrites(SpeConn_t* conn, char* buf) {
  ASSERT(conn && buf);
  if (conn->Closed || conn->Error) return false;
  return SpeStringCatb(conn->writeBuffer, buf, strlen(buf));
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
