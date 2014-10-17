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

static SpeConn_t all_conn[MAX_FD];

static void
connectNormal(void* arg) {
  SpeConn_t* conn = arg;
  // connect timeout
  if (conn->readExpireTime && conn->readTask.Timeout) {
    conn->ConnectTimeout = 1;
    goto end_out;
  }
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->Error = 1;
  if (err) conn->Error = 1;
end_out:
  SpeEpollDisable(conn->fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  if (conn->readExpireTime) SpeTaskDequeueTimer(&conn->readTask);
  conn->readType  = SPE_CONN_READNONE;
  conn->writeType = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->ReadCallback.Handler);
}

/*
===================================================================================================
SpeConnConnect
===================================================================================================
*/
bool
SpeConnConnect(SpeConn_t* conn, const char* addr, const char* port) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE && conn->writeType == SPE_CONN_WRITENONE && 
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
  if (connect(conn->fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      conn->readTask.Handler  = SPE_HANDLER1(connectNormal, conn);
      conn->readTask.Timeout  = 0;
      conn->readType          = SPE_CONN_CONNECT;
      conn->writeType         = SPE_CONN_CONNECT;
      conn->ConnectTimeout    = 0;
      SpeEpollEnable(conn->fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE, &conn->readTask);
      if (conn->readExpireTime) SpeTaskEnqueueTimer(&conn->readTask, conn->readExpireTime);
      freeaddrinfo(servinfo);
      return true;
    }
    conn->Error = 1;
  }
  // (sync): connect success or failed, call handler
  SpeTaskEnqueue(&conn->ReadCallback);
  freeaddrinfo(servinfo);
  return true;
}

/*
===================================================================================================
readNormal
===================================================================================================
*/
static void
readNormal(void* arg) {
  SpeConn_t* conn = arg;
  // check timeout
  if (conn->readExpireTime && conn->readTask.Timeout) {
    conn->ReadTimeout = 1;
    goto end_out;
  }
  // read data
  for (;;) {
    int res = SpeBufReadAppend(conn->fd, conn->ReadBuffer, BUF_SIZE);
    // read error
    if (res == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      SPE_LOG_ERR("conn error: %s", strerror(errno));
      conn->Error = 1;
      break;
    }
    // peer close
    if (res == 0) {
      conn->Closed = 1;
      break;
    }
    break;
  }
  // check read type
  if (conn->readType == SPE_CONN_READUNTIL) {
    int pos = SpeBufSearch(conn->ReadBuffer, conn->delim);
    if (pos != -1) {
      conn->RLen = pos + strlen(conn->delim);
      goto end_out;
    }
  } else if (conn->readType == SPE_CONN_READBYTES) {
    if (conn->rbytes <= conn->ReadBuffer->Len) {
      conn->RLen = conn->rbytes;
      goto end_out;
    }
  } else if (conn->readType == SPE_CONN_READ) {
    if (conn->ReadBuffer->Len > 0) { 
      conn->RLen = conn->ReadBuffer->Len;
      goto end_out;
    }
  }
  // check error and close 
  if (conn->Closed || conn->Error) goto end_out;
  return;

end_out:
  SpeEpollDisable(conn->fd, SPE_EPOLL_READ);
  if (conn->readExpireTime) SpeTaskDequeueTimer(&conn->readTask);
  conn->readType = SPE_CONN_READNONE;
  SPE_HANDLER_CALL(conn->ReadCallback.Handler);
}

/*
===================================================================================================
SpeConnRead_until
===================================================================================================
*/
bool
SpeConnReaduntil(SpeConn_t* conn, char* delim) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE);
  if (!delim || conn->Closed || conn->Error) return false;
  // (sync):
  int pos = SpeBufSearch(conn->ReadBuffer, delim);
  if (pos != -1) {
    conn->RLen = pos + strlen(delim);
    SpeTaskEnqueue(&conn->ReadCallback);
    return true;
  }
  // (async):
  conn->readTask.Handler  = SPE_HANDLER1(readNormal, conn);
  conn->readTask.Timeout  = 0;
  conn->ReadTimeout       = 0;
  conn->RLen              = 0;
  conn->delim             = delim;
  conn->readType          = SPE_CONN_READUNTIL;
  SpeEpollEnable(conn->fd, SPE_EPOLL_READ, &conn->readTask);
  if (conn->readExpireTime) SpeTaskEnqueueTimer(&conn->readTask, conn->readExpireTime);
  return true;
}

/*
===================================================================================================
SpeConnReadbytes
===================================================================================================
*/
bool
SpeConnReadbytes(SpeConn_t* conn, unsigned len) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE);
  if (len == 0 || conn->Closed || conn->Error ) return false;
  // (sync):
  if (len <= conn->ReadBuffer->Len) {
    conn->RLen = len;
    SpeTaskEnqueue(&conn->ReadCallback);
    return true;
  }
  // (async):
  conn->readTask.Handler  = SPE_HANDLER1(readNormal, conn);
  conn->readTask.Timeout  = 0;
  conn->ReadTimeout       = 0;
  conn->RLen              = 0;
  conn->rbytes            = len;
  conn->readType          = SPE_CONN_READBYTES;
  SpeEpollEnable(conn->fd, SPE_EPOLL_READ, &conn->readTask);
  if (conn->readExpireTime) SpeTaskEnqueueTimer(&conn->readTask, conn->readExpireTime);
  return true;
}

/*
===================================================================================================
SpeConnRead
===================================================================================================
*/
bool
SpeConnRead(SpeConn_t* conn) {
  ASSERT(conn && conn->readType == SPE_CONN_READNONE);
  if (conn->Closed || conn->Error) return false;
  // (sync):
  if (conn->ReadBuffer->Len > 0) {
    conn->RLen = conn->ReadBuffer->Len;
    SpeTaskEnqueue(&conn->ReadCallback);
    return true;
  }
  // (async):
  conn->readTask.Handler  = SPE_HANDLER1(readNormal, conn);
  conn->readTask.Timeout  = 0;
  conn->ReadTimeout       = 0;
  conn->RLen              = 0;
  conn->readType          = SPE_CONN_READ;
  SpeEpollEnable(conn->fd, SPE_EPOLL_READ, &conn->readTask);
  if (conn->readExpireTime) SpeTaskEnqueueTimer(&conn->readTask, conn->readExpireTime);
  return true;
}

/*
===================================================================================================
writeNormal
===================================================================================================
*/
static void
writeNormal(void* arg) {
  SpeConn_t* conn = arg;
  // check timeout
  if (conn->writeExpireTime && conn->writeTask.Timeout) {
    conn->WriteTimeout = 1;
    goto end_out;
  }
  // write date
  int res = write(conn->fd, conn->writeBuffer->Data, conn->writeBuffer->Len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->Closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->Error = 1;
    }
    goto end_out;
  }
  SpeBufLConsume(conn->writeBuffer, res);
  if (conn->writeBuffer->Len == 0) goto end_out;
  return;

end_out:
  SpeEpollDisable(conn->fd, SPE_EPOLL_WRITE);
  if (conn->writeExpireTime) SpeTaskDequeueTimer(&conn->writeTask);
  conn->writeType = SPE_CONN_WRITENONE;
  SPE_HANDLER_CALL(conn->WriteCallback.Handler);
}

/*
===================================================================================================
SpeConnFlush
===================================================================================================
*/
bool
SpeConnFlush(SpeConn_t* conn) {
  ASSERT(conn && conn->writeType == SPE_CONN_WRITENONE);
  if (conn->Closed || conn->Error) return false;
  if (conn->writeBuffer->Len == 0) {
    SpeTaskEnqueue(&conn->WriteCallback);
    return true;
  }
  conn->writeTask.Handler = SPE_HANDLER1(writeNormal, conn);
  conn->writeTask.Timeout = 0;
  conn->writeType         = SPE_CONN_WRITE;
  SpeEpollEnable(conn->fd, SPE_EPOLL_WRITE, &conn->writeTask);
  if (conn->writeExpireTime) SpeTaskEnqueueTimer(&conn->writeTask, conn->writeExpireTime);
  return true;
}

/*
===================================================================================================
SpeConnSetTimeout
===================================================================================================
*/
bool
SpeConnSetTimeout(SpeConn_t* conn, unsigned readExpireTime, unsigned writeExpireTime) {
  ASSERT(conn);
  conn->readExpireTime  = readExpireTime;
  conn->writeExpireTime = writeExpireTime;
  return true;
}

/*
===================================================================================================
connInit
===================================================================================================
*/
static bool
connInit(SpeConn_t* conn, unsigned fd) {
  conn->fd = fd;
  SpeTaskInit(&conn->readTask, SPE_TASK_NORM);
  SpeTaskInit(&conn->writeTask, SPE_TASK_NORM);
  SpeTaskInit(&conn->ReadCallback, SPE_TASK_NORM);
  SpeTaskInit(&conn->WriteCallback, SPE_TASK_NORM);
  conn->ReadBuffer  = SpeBufCreate();
  conn->writeBuffer = SpeBufCreate();
  if (!conn->ReadBuffer || !conn->writeBuffer) {
    SpeBufDestroy(conn->ReadBuffer);
    SpeBufDestroy(conn->writeBuffer);
    return false;
  }
  return true;
}

/*
===================================================================================================
SpeConnCreate
===================================================================================================
*/
SpeConn_t*
SpeConnCreate(unsigned fd) {
  if (unlikely(fd >= MAX_FD)) {
    SPE_LOG_ERR("conn overflow");
    return NULL;
  }
  SpeSockSetBlock(fd, 0);
  SpeConn_t* conn = &all_conn[fd];
  if (conn->fd == 0 && !connInit(conn, fd)) {
    SPE_LOG_ERR("connInit error");
    return NULL;
  }
  SpeBufClean(conn->ReadBuffer);
  SpeBufClean(conn->writeBuffer);
  // init conn status
  conn->readExpireTime  = 0;
  conn->writeExpireTime = 0;
  conn->readType        = SPE_CONN_READNONE;
  conn->writeType       = SPE_CONN_WRITENONE;
  conn->Closed          = 0;
  conn->Error           = 0;
  return conn;
}

/*
===================================================================================================
SpeConnDestroy
===================================================================================================
*/
void
SpeConnDestroy(SpeConn_t* conn) {
  ASSERT(conn);
  SpeTaskDequeue(&conn->readTask);
  SpeTaskDequeue(&conn->writeTask);
  SpeTaskDequeue(&conn->ReadCallback);
  SpeTaskDequeue(&conn->WriteCallback);
  if (conn->readExpireTime) SpeTaskDequeueTimer(&conn->readTask);
  if (conn->writeExpireTime) SpeTaskDequeueTimer(&conn->writeTask);
  SpeEpollDisable(conn->fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  SpeSockClose(conn->fd);
}
