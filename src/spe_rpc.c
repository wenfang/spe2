#include "spe_rpc.h"
#include "spe_server.h"
#include <stdlib.h>

#define SPE_RPC_MAPLEN  31

#define RPC_PARSE_STAR  1
#define RPC_PARSE_PARM  2
#define RPC_PARSE_DATA  3
#define RPC_PARSE_END   4

static char* cmdNotSupportErr = "Command Not Support";

static int checkToken(speConn_t* conn) {
  int pos = -1;
  if (conn->ReadBuffer->Len == 0) return pos;
  for (int i=0; i<conn->ReadBuffer->Len-1; i++) {
    if (conn->ReadBuffer->Data[i] == '\r' && conn->ReadBuffer->Data[i+1] == '\n') {
      pos = i;
      break;
    }
  }
  return pos;
}

static void
rpcProcess(speRPCConn_t* rpcConn) {
  SpeRPCHandler handle = SpeMapGet(rpcConn->rpc->cmdMap, (const char*)rpcConn->request->Data[0]->Data);
  SpeBufsClean(rpcConn->response.msg);
  if (handle) {
    handle(rpcConn);
  } else {
    rpcConn->response.type = SPE_RPCMSG_ERR;
    SpeBufsAppend(rpcConn->response.msg, cmdNotSupportErr, strlen(cmdNotSupportErr));
    SpeRPCDone(rpcConn);
  }
}

static void
rpcParse(speRPCConn_t* rpcConn) {
  if (rpcConn->conn->Closed || rpcConn->conn->Error) {
    goto errout;
  }
  int pos = checkToken(rpcConn->conn);
  while (pos != -1) {
    switch (rpcConn->status) {
      case RPC_PARSE_STAR:
        if (pos<2 || rpcConn->conn->ReadBuffer->Data[0] != '*') goto errout;
        rpcConn->conn->ReadBuffer->Data[pos] = 0;
        rpcConn->paramLeft = atoi(rpcConn->conn->ReadBuffer->Data+1);
        if (rpcConn->paramLeft < 1) goto errout;
        SpeBufLConsume(rpcConn->conn->ReadBuffer, pos+2);  
        rpcConn->status = RPC_PARSE_PARM;
        break;
      case RPC_PARSE_PARM:
        if (pos<2 || rpcConn->conn->ReadBuffer->Data[0] != '$') goto errout;
        rpcConn->conn->ReadBuffer->Data[pos] = 0;
        rpcConn->dataLeft = atoi(rpcConn->conn->ReadBuffer->Data+1);
        if (rpcConn->dataLeft < 0) goto errout;
        SpeBufLConsume(rpcConn->conn->ReadBuffer, pos+2);
        rpcConn->status = RPC_PARSE_DATA;
        break;
      case RPC_PARSE_DATA: 
        if (pos > rpcConn->dataLeft) goto errout;
        if (pos < rpcConn->dataLeft) {
          SpeBufsAppend(rpcConn->request, rpcConn->conn->ReadBuffer->Data, pos+2);
          SpeBufLConsume(rpcConn->conn->ReadBuffer, pos+2);
          rpcConn->dataLeft -= pos+2;
          break;
        }
        SpeBufsAppend(rpcConn->request, rpcConn->conn->ReadBuffer->Data, pos);
        SpeBufLConsume(rpcConn->conn->ReadBuffer, pos+2);
        rpcConn->paramLeft--;
        if (rpcConn->paramLeft == 0) {
          rpcConn->status = RPC_PARSE_STAR;
          rpcProcess(rpcConn);
          return;
        } else {
          rpcConn->status = RPC_PARSE_PARM;
        }
      break;
    }
    pos = checkToken(rpcConn->conn);
  }
  SpeConnRead(rpcConn->conn);
  return;

errout:
  SpeConnDestroy(rpcConn->conn);
  SpeBufsDestroy(rpcConn->request);
  SpeBufsDestroy(rpcConn->response.msg);
  free(rpcConn);
  return;
}

static void
rpcHandler(speConn_t* conn, void* rpc) {
  speRPCConn_t* rpcConn = calloc(1, sizeof(speRPCConn_t));
  if (!rpcConn) goto errout1;
  rpcConn->request = SpeBufsCreate();
  if (!rpcConn->request) goto errout2;
  rpcConn->response.msg = SpeBufsCreate();
  if (!rpcConn->response.msg) goto errout3;

  rpcConn->status = RPC_PARSE_STAR;
  rpcConn->conn   = conn;
  rpcConn->rpc    = rpc;
  conn->PostReadTask.Handler  = SPE_HANDLER1(rpcParse, rpcConn);
  conn->PostWriteTask.Handler = SPE_HANDLER_NULL;
  SpeConnRead(conn);
  return;

errout3:
  SpeBufsDestroy(rpcConn->request);
errout2:
  free(rpcConn);
errout1:
  SpeConnDestroy(conn);
  return;
}

speRPC_t*
SpeRPCCreate(const char* addr, int port) {
  speRPC_t* rpc = calloc(1, sizeof(speRPC_t));
  if (!rpc) return NULL;
  rpc->cmdMap = SpeMapCreate(SPE_RPC_MAPLEN, NULL);
  if (!rpc->cmdMap) {
    free(rpc);
    return NULL;
  }
  rpc->server = SpeServerRegister(addr, port, rpcHandler, rpc);
  return rpc;
}

void
SpeRPCDestroy(speRPC_t* rpc) {
  if (!rpc) return;
  SpeMapDestroy(rpc->cmdMap);
  SpeServerUnregister(rpc->server);
  free(rpc);
}

bool
SpeRPCRegisteHandler(speRPC_t *rpc, const char* cmd, SpeRPCHandler handler) {
  ASSERT(rpc && cmd && handler);
  int rc = SpeMapSet(rpc->cmdMap, cmd, handler);
  if (rc != SPE_MAP_OK) return false;
  return true;
}

void
SpeRPCDone(speRPCConn_t* rpcConn) {
  char buf[1024] = {0};
  speBufs_t* rspMsg = rpcConn->response.msg;
  switch(rpcConn->response.type) {
    case SPE_RPCMSG_OK:
      SpeConnWrite(rpcConn->conn, "+", 1);
      SpeConnWrite(rpcConn->conn, rspMsg->Data[0]->Data, rspMsg->Data[0]->Len);
      SpeConnWrite(rpcConn->conn, "\r\n", 2);
      break;
    case SPE_RPCMSG_ERR:
      SpeConnWrite(rpcConn->conn, "-", 1);
      SpeConnWrite(rpcConn->conn, rspMsg->Data[0]->Data, rspMsg->Data[0]->Len);
      SpeConnWrite(rpcConn->conn, "\r\n", 2);
      break;
    case SPE_RPCMSG_NUM:
      SpeConnWrite(rpcConn->conn, ":", 1);
      sprintf(buf, "%ld\r\n", strtol(rspMsg->Data[0]->Data, NULL, 10));
      SpeConnWrite(rpcConn->conn, buf, strlen(buf));
      break;
    case SPE_RPCMSG_BULK:
      break;
    case SPE_RPCMSG_MBULK:
      break;
  }
  SpeConnFlush(rpcConn->conn);
  SpeBufsClean(rpcConn->request);
  SpeConnRead(rpcConn->conn);
}
