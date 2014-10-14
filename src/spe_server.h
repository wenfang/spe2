#ifndef __SPE_SERVER_H
#define __SPE_SERVER_H

#include "spe_conn.h"
#include "spe_module.h"
#include <stdbool.h>

typedef void (*SpeServerHandler)(SpeConn_t*);

extern void
SpeServerPreLoop();

extern void
SpeServerPostLoop();

extern bool
SpeServerRegister(const char* addr, int port, SpeServerHandler handler);

extern speModule_t speServerModule;

#endif
