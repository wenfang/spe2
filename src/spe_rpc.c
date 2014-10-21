#include "spe_rpc.h"
#include "spe_server.h"
#include <stdlib.h>

#define SPE_RPC_MAPLEN  31

static void
rpcHandler(SpeConn_t* conn) {
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
  SpeServerRegister(addr, port, rpcHandler);
  return rpc;
}

void
SpeRPCDestroy(speRPC_t* rpc) {
  if (!rpc) return;
  SpeMapDestroy(rpc->cmdMap);
  free(rpc);
}
