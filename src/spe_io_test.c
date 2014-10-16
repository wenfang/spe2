#include "spe_io.h"
#include <stdio.h>

int main() {
  speIO_t* io = SpeIOCreate("/test.data");
  speBuf_t* buf = SpeBufCreate();

  for (;;) {
    SpeIOReaduntil(io, "\n");
    if (io->Error || io->Closed) break;
    SpeBufCopy(buf, io->ReadBuffer->Data, io->RLen - strlen("\n"));
    SpeBufLConsume(io->ReadBuffer, io->RLen);
    printf("%s\n", buf->Data);
  }
  
  SpeIODestroy(io);
  return 1;
}
