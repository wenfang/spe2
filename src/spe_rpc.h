#ifndef __SPE_RPC_H
#define __SPE_RPC_H

#include "spe_map.h"
#include "spe_buf.h"
#include "spe_conn.h"

#define SPE_RPCMSG_NONE   0
#define SPE_RPCMSG_OK     1
#define SPE_RPCMSG_ERR    2
#define SPE_RPCMSG_NUM    3
#define SPE_RPCMSG_BULK   4
#define SPE_RPCMSG_MBULK  5

typedef struct {
  speMap_t* cmdMap;
} speRPC_t;

typedef struct {
  int           type;
  speBufList_t* msg;
} speRPCResp_t;

typedef struct {
  unsigned      status;
  int           paramLeft;
  int           dataLeft;
  speBufList_t* request;
  speRPCResp_t  response;
  speConn_t*    conn;
  speRPC_t*     rpc;
} speRPCConn_t;

typedef void (*SpeRPCHandler)(speRPCConn_t* rpcConn);

extern speRPC_t*
SpeRPCCreate(const char* addr, int port);

extern void
SpeRPCDestroy(speRPC_t *rpc);

extern bool
SpeRPCRegisteHandler(speRPC_t *rpc, const char* cmd, SpeRPCHandler handler);

extern void
SpeRPCDone(speRPCConn_t* rpcConn);

#endif
