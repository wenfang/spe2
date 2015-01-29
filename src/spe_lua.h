#ifndef __SPE_LUA_H
#define __SPE_LUA_H

#include "spe_module.h"

extern void
SpeLuaRun(const char* buff);

extern void
SpeLuaThread(const char* fname);

extern speModule_t speLuaModule;

#endif
