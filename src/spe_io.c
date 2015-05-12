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
#define SPE_IO_READUNTIL  2

/*
===================================================================================================
read_common
===================================================================================================
*/
static int
read_common(spe_io_t* io, spe_buf_t *buf) {
  int res;
  for (;;) {
    if (io->rtype == SPE_IO_READ) {
      if (io->rbytes <= io->read_buffer->Len) {
        SpeBufCopy(buf, io->read_buffer->Data, io->rbytes);
        SpeBufLConsume(io->read_buffer, io->rbytes);
        io->rtype = SPE_IO_READNONE;
        return io->rbytes;
      }
    } else if (io->rtype == SPE_IO_READUNTIL) {
      int pos = SpeBufSearch(io->read_buffer, io->delim);
      if (pos != -1) {
        unsigned len = pos + strlen(io->delim);
        SpeBufCopy(buf, io->read_buffer->Data, len);
        SpeBufLConsume(io->read_buffer, len);
        io->rtype = SPE_IO_READNONE;
        return len;
      }
    }
    res = SpeBufReadAppend(io->fd, io->read_buffer, BUF_LEN);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->error = 1;
      break;
    }
    if (res == 0) {
      io->closed = 1;
      break;
    }
  }
  // read error copy data
  SpeBufCopy(buf, io->read_buffer->Data, io->read_buffer->Len);
  SpeBufLConsume(io->read_buffer, io->read_buffer->Len);
  io->rtype = SPE_IO_READNONE;
  return res;
}

/*
===================================================================================================
spe_io_read
===================================================================================================
*/
int 
spe_io_read(spe_io_t* io, unsigned len, spe_buf_t *buf) {
  ASSERT(io && len && buf);
  if (io->closed || io->error) return -1;
  io->rtype   = SPE_IO_READ;
  io->rbytes  = len;
  return read_common(io, buf);
}

/*
===================================================================================================
spe_io_read_until
===================================================================================================
*/
int 
spe_io_read_until(spe_io_t* io, const char* delim, spe_buf_t *buf) {  
  ASSERT(io && delim);
  if (io->closed || io->error) return -1;
  io->rtype = SPE_IO_READUNTIL;
  io->delim = delim;
  return read_common(io, buf);
}

/*
===================================================================================================
spe_io_write
===================================================================================================
*/
int
spe_io_write(spe_io_t* io, spe_buf_t *buf) {
  ASSERT(io && buf);
  if (io->closed || io->error) return -1;
  int totalWrite = 0;
  while (totalWrite < buf->Len) {
    int res = write(io->fd, buf->Data+totalWrite, buf->Len-totalWrite);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->error = 1;
      break;
    }
    totalWrite += res;
  }
  return totalWrite;
}

/*
===================================================================================================
spe_io_create
===================================================================================================
*/
spe_io_t*
spe_io_create(const char* fname) {
  ASSERT(fname);
  spe_io_t* io = calloc(1, sizeof(spe_io_t));
  if (io == NULL) return NULL;

  if ((io->fd = open(fname, O_RDWR)) < 0) {
    free(io);
    return NULL;
  }
  io->read_buffer = SpeBufCreate();
  if (io->read_buffer == NULL) {
    spe_io_destroy(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
spe_io_create_fd
===================================================================================================
*/
spe_io_t*
spe_io_create_fd(int fd) {
  spe_io_t* io = calloc(1, sizeof(spe_io_t));
  if (io == NULL) return NULL;

  io->fd          = fd;
  io->read_buffer = SpeBufCreate();
  if (io->read_buffer == NULL) {
    SpeBufDestroy(io->read_buffer);
    free(io);
    return NULL;
  }
  return io;
}

/*
===================================================================================================
spe_io_destroy
===================================================================================================
*/
void 
spe_io_destroy(spe_io_t* io) {
  ASSERT(io);
  SpeBufDestroy(io->read_buffer);
  close(io->fd);
  free(io);
}
