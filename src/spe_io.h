#ifndef __SPE_IO_H
#define __SPE_IO_H

#include "spe_buf.h"

typedef struct {
  speBuf_t*   ReadBuffer;
  speBuf_t*   writeBuffer;
  unsigned    RLen;
  const char* delim;
  unsigned    fd;
  unsigned    rbytes;
  unsigned    rtype:2;
  unsigned    Closed:1;
  unsigned    Error:1;
} speIO_t;

extern int
SpeIORead(speIO_t* io);

extern int  
SpeIOReaduntil(speIO_t* io, const char* delim);

extern int  
SpeIOReadbytes(speIO_t* io, unsigned len);

static inline bool
SpeIOWrite(speIO_t* io, char* src, unsigned len) {
  ASSERT(io && src);
  if (io->Closed || io->Error) return false;
  return SpeBufCat(io->writeBuffer, src, len);
}

extern int
SpeIOFlush(speIO_t* io);

extern speIO_t*
SpeIOCreate(const char* fname);

extern speIO_t*
SpeIOCreateFd(int fd);

extern void
SpeIODestroy(speIO_t* io);

#endif
