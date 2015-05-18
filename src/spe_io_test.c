#include "spe_io.h"
#include <stdio.h>

int main() {
  spe_io_t* io = spe_io_create("test/iotest.data");
  spe_buf_t* buf = spe_buf_create();

  for (;;) {
    int rc = spe_io_readuntil(io, "t", buf);
    printf("rc: %d\n", rc);
    if (buf->len) printf("%s\n", buf->data);
    spe_buf_clean(buf);
    if (rc <= 0) break;
  }
  
  spe_io_destroy(io);
  return 1;
}
