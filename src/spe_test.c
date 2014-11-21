#include "spe.h"

static int count = 0;

static void onClose(speConn_t *conn) {
  SpeConnDestroy(conn);
}

static void onRead(speConn_t *conn) {
  if (count++ > 100000) speWorkerStop=true;
  char* str = "HTTP/1.1 220 OK\r\n\r\nOK\r\n";
  SpeConnWrite(conn, str, strlen(str));
  conn->WriteCallback.Handler = SPE_HANDLER1(onClose, conn);
  SpeConnFlush(conn);
}

static void process(speConn_t *conn, void* arg) {
  conn->ReadCallback.Handler = SPE_HANDLER1(onRead, conn);
  SpeConnReaduntil(conn, "\r\n\r\n");
}

/*
static void 
infoHandle(speRPCConn_t* rpcConn) {
  rpcConn->response.type = SPE_RPCMSG_NUM;
  SpeBufListAppend(rpcConn->response.msg, "10", 2);
  SpeRPCDone(rpcConn);
  SpeLuaThread("test.lua");
}

static speRPC_t* rpc;
*/

static void
testInit() {
  /*
  rpc = SpeRPCCreate("127.0.0.1", 7879);
  SpeRPCRegisteHandler(rpc, "info", infoHandle);
  */
  SpeServerRegister("127.0.0.1", 7879, process, NULL);
}

static void
testDeinit() {
  /*
  SpeRPCDestroy(rpc);
  */
}

speModule_t speTestModule = {
  "speTest",
  0,
  SPE_USER_MODULE,
  testInit,
  NULL,
  NULL,
  testDeinit,
};
