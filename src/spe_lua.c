#include "spe_lua.h"
#include "spe_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lua_State *L;

void 
SpeLuaRun(const char* buff) {
  luaL_loadbuffer(L, buff, strlen(buff), "line");
  lua_pcall(L, 0, 0, 0);
}

SpeLuaThread_t*
SpeLuaThread(const char* fname) {
  SpeLuaThread_t* thread = calloc(1, sizeof(SpeLuaThread_t));
  if (!thread) return NULL;
  thread->state = lua_newthread(L);
  luaL_loadfile(thread->state, fname);
  return thread;
}

static bool
luaInit(speCycle_t *cycle) {
  L = luaL_newstate();
  if (!L) {
    SPE_LOG_ERR("luaL_newstate error");
    return false;
  }
  luaL_openlibs(L);
  return true;
}

static bool
luaExit(speCycle_t *cycle) {
  lua_close(L);
  return true;
}

speModule_t speLuaModule = {
  "speLua",
  0,
  SPE_CORE_MODULE,
  luaInit,
  NULL,
  NULL,
  luaExit,
};
