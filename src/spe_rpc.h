#ifndef __SPE_RPC_H
#define __SPE_RPC_H

#include "spe_map.h"

typedef struct {
} speRPCMsg_t;

typedef struct {
  speMap_t* cmdMap;
} speRPC_t;

extern speRPC_t*
SpeRPCCreate(const char* addr, int port);

extern void
SpeRPCDestroy(speRPC_t *rpc);

extern bool
SpeRPCRegisteHandler(speRPC_t *rpc, const char* cmd);

#endif
