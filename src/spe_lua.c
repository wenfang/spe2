#include "spe_lua.h"
#include "spe_log.h"
#include <stdio.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static lua_State *L;

void 
SpeLuaRun(const char* buff) {
  luaL_loadbuffer(L, buff, strlen(buff), "line");
  lua_pcall(L, 0, 0, 0);
  lua_getglobal(L, "X");
  int x = lua_tointeger(L, -1);
  SPE_LOG_ERR("GET FROM LUA: %d", x);
}

void
SpeLuaThread(const char* fname) {
  lua_State *L1 = lua_newthread(L);
  luaL_loadfile(L1, fname);
  lua_resume(L1, L, 0);
  lua_resume(L1, L, 0);
  lua_resume(L1, L, 0);
  lua_resume(L1, L, 0);
  lua_getglobal(L1, "X");
  int x = lua_tointeger(L1, -1);
  SPE_LOG_ERR("GET FROM LUA: %d", x);
}

static bool
luaInit(speCycle_t *cycle) {
  L = luaL_newstate();
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
  NULL,
  luaInit,
  luaExit,
  NULL,
};
