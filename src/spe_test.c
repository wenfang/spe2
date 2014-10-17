#include "spe_test.h"
#include "spe_server.h"
#include "spe_worker.h"

static void onClose(SpeConn_t *conn) {
  SpeConnDestroy(conn);
}

static void onRead(SpeConn_t *conn) {
  char* str = "OK\r\n";
  SpeConnWrite(conn, str, strlen(str));
  conn->WriteCallback.Handler = SPE_HANDLER1(onClose, conn);
  SpeConnFlush(conn);
}

static void process(SpeConn_t *conn) {
  conn->ReadCallback.Handler = SPE_HANDLER1(onRead, conn);
  SpeConnReaduntil(conn, "\r\n\r\n");
}

static void
testInit() {
  SpeServerRegister("127.0.0.1", 7879, process);
}

speModule_t speTestModule = {
  "speTest",
  0,
  SPE_USER_MODULE,
  testInit,
  NULL,
  NULL,
  NULL,
};
