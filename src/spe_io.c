#include "spe_io.h"
#include "spe_util.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_LEN 4096

#define SPE_IO_READNONE   0
#define SPE_IO_READ       1
#define SPE_IO_READBYTES  2
#define SPE_IO_READUNTIL  3

/*
===================================================================================================
ioReadCommon
===================================================================================================
*/
static int
ioReadCommon(speIO_t* io) {
  int res;
  for (;;) {
    if (io->rtype == SPE_IO_READ) {
      if (io->ReadBuffer->Len > 0) {
        io->RLen = io->ReadBuffer->Len;
        io->rtype = SPE_IO_READNONE;
        return io->RLen;
      }
    } else if (io->rtype == SPE_IO_READBYTES) {
      if (io->rbytes <= io->ReadBuffer->Len) {
        io->RLen = io->rbytes;
        io->rtype = SPE_IO_READNONE;
        return io->RLen;
      }
    } else if (io->rtype == SPE_IO_READUNTIL) {
      int pos = SpeBufSearch(io->ReadBuffer, io->delim);
      if (pos != -1) {
        io->RLen = pos + strlen(io->delim);
        io->rtype = SPE_IO_READNONE;
        return io->RLen;
      }
    }
    res = SpeBufReadAppend(io->fd, io->ReadBuffer, BUF_LEN);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->Error = 1;
      break;
    }
    if (res == 0) {
      io->Closed = 1;
      break;
    }
  }
  // read error copy data
  io->RLen  = io->ReadBuffer->Len;
  io->rtype = SPE_IO_READNONE;
  return res;
}

/*
===================================================================================================
SpeIORead
===================================================================================================
*/
int
SpeIORead(speIO_t* io) {
  ASSERT(io);
  if (io->Closed || io->Error) return -1;
  io->rtype = SPE_IO_READ;
  return ioReadCommon(io);
}

/*
===================================================================================================
spe_io_readbytes
===================================================================================================
*/
int 
SpeIOReadbytes(speIO_t* io, unsigned len) {
  ASSERT(io && len);
  if (io->Closed || io->Error) return -1;
  io->rtype  = SPE_IO_READBYTES;
  io->rbytes = len;
  return ioReadCommon(io);
}

/*
===================================================================================================
SpeIOReaduntil
===================================================================================================
*/
int 
SpeIOReaduntil(speIO_t* io, const char* delim) {  
  ASSERT(io && delim);
  if (io->Closed || io->Error) return -1;
  io->rtype  = SPE_IO_READUNTIL;
  io->delim  = delim;
  return ioReadCommon(io);
}

/*
===================================================================================================
SpeIOFlush
===================================================================================================
*/
int
SpeIOFlush(speIO_t* io) {
  ASSERT(io);
  if (io->Closed || io->Error) return -1;
  int totalWrite = 0;
  while (totalWrite < io->writeBuffer->Len) {
    int res = write(io->fd, io->writeBuffer->Data+totalWrite, io->writeBuffer->Len-totalWrite);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->Error = 1;
      break;
    }
    totalWrite += res;
  }
  return totalWrite;
}

/*
===================================================================================================
SpeIOCreate
===================================================================================================
*/
speIO_t*
SpeIOCreate(const char* fname) {
  ASSERT(fname);
  speIO_t* io = calloc(1, sizeof(speIO_t));
  if (!io) return NULL;
  if ((io->fd = open(fname, O_RDWR)) < 0) {
    free(io);
    return NULL;
  }
  io->ReadBuffer  = SpeBufCreate();
  io->writeBuffer = SpeBufCreate();
  if (!io->ReadBuffer || !io->writeBuffer) {
    SpeIODestroy(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
SpeIOCreateFd
===================================================================================================
*/
speIO_t*
SpeIOCreateFd(int fd) {
  speIO_t* io = calloc(1, sizeof(speIO_t));
  if (!io) return NULL;
  io->fd          = fd;
  io->ReadBuffer  = SpeBufCreate();
  io->writeBuffer = SpeBufCreate();
  if (!io->ReadBuffer || !io->writeBuffer) {
    SpeBufDestroy(io->ReadBuffer);
    SpeBufDestroy(io->writeBuffer);
    free(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
SpeIODestroy
===================================================================================================
*/
void 
SpeIODestroy(speIO_t* io) {
  ASSERT(io);
  SpeBufDestroy(io->ReadBuffer);
  SpeBufDestroy(io->writeBuffer);
  close(io->fd);
  free(io);
}
