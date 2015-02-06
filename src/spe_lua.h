#ifndef __SPE_LUA_H
#define __SPE_LUA_H

#include "spe_module.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

typedef struct {
  lua_State *state;
} SpeLuaThread_t;

extern void
SpeLuaRun(const char* buff);

extern SpeLuaThread_t*
SpeLuaThread(const char* fname);

extern speModule_t speLuaModule;

#endif
