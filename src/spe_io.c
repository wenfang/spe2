#include "spe_io.h"
#include "spe_util.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_LEN 4096

#define SPE_IO_READNONE     0
#define SPE_IO_READ         1
#define SPE_IO_READBYTES    2
#define SPE_IO_READUNTIL    3

/*
===================================================================================================
read_common
===================================================================================================
*/
static int
read_common(spe_io_t* io, spe_buf_t *buf) {
  int res;
  for (;;) {
    if (io->_rtype == SPE_IO_READ) {
      if (io->_read_buffer->len > 0) {
        unsigned len = io->_read_buffer->len;
        spe_buf_append(buf, io->_read_buffer->data, io->_read_buffer->len);
        spe_buf_clean(io->_read_buffer);
        io->_rtype = SPE_IO_READNONE;
        return len;
      }
    } else if (io->_rtype == SPE_IO_READBYTES) {
      if (io->_rbytes <= io->_read_buffer->len) {
        spe_buf_append(buf, io->_read_buffer->data, io->_rbytes);
        spe_buf_lconsume(io->_read_buffer, io->_rbytes);
        io->_rtype = SPE_IO_READNONE;
        return io->_rbytes;
      }
    } else if (io->_rtype == SPE_IO_READUNTIL) {
      int pos = spe_buf_search(io->_read_buffer, io->_delim);
      if (pos != -1) {
        unsigned len = pos + strlen(io->_delim);
        spe_buf_append(buf, io->_read_buffer->data, len);
        spe_buf_lconsume(io->_read_buffer, len);
        io->_rtype = SPE_IO_READNONE;
        return len;
      }
    }
    res = spe_buf_read_fd_append(io->_fd, BUF_LEN, io->_read_buffer);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->_error = 1;
      break;
    }
    if (res == 0) {
      io->_closed = 1;
      break;
    }
  }
  // read error copy data
  spe_buf_append(buf, io->_read_buffer->data, io->_read_buffer->len);
  spe_buf_clean(io->_read_buffer);
  io->_rtype = SPE_IO_READNONE;
  return res;
}

/*
===================================================================================================
spe_io_read
===================================================================================================
*/
int 
spe_io_read(spe_io_t* io, spe_buf_t *buf) {
  ASSERT(io && buf);
  if (io->_closed) return SPE_IO_CLOSED;
  if (io->_error) return SPE_IO_ERROR;
  io->_rtype  = SPE_IO_READ;
  return read_common(io, buf);
}

/*
===================================================================================================
spe_io_readbytes
===================================================================================================
*/
int 
spe_io_readbytes(spe_io_t* io, unsigned len, spe_buf_t *buf) {
  ASSERT(io && len && buf);
  if (io->_closed) return SPE_IO_CLOSED;
  if (io->_error) return SPE_IO_ERROR;
  io->_rtype  = SPE_IO_READBYTES;
  io->_rbytes = len;
  return read_common(io, buf);
}

/*
===================================================================================================
spe_io_readuntil
===================================================================================================
*/
int 
spe_io_readuntil(spe_io_t* io, const char* delim, spe_buf_t *buf) {  
  ASSERT(io && delim);
  if (io->_closed) return SPE_IO_CLOSED;
  if (io->_error) return SPE_IO_ERROR;
  io->_rtype = SPE_IO_READUNTIL;
  io->_delim = delim;
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
  if (io->_closed || io->_error) return SPE_IO_ERROR;
  int total_write = 0;
  while (total_write < buf->len) {
    int res = write(io->_fd, buf->data+total_write, buf->len-total_write);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->_error = 1;
      break;
    }
    total_write += res;
  }
  return total_write;
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

  if ((io->_fd = open(fname, O_RDWR)) < 0) {
    free(io);
    return NULL;
  }
  io->_read_buffer = spe_buf_create();
  if (io->_read_buffer == NULL) {
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

  io->_fd          = fd;
  io->_read_buffer = spe_buf_create();
  if (io->_read_buffer == NULL) {
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
  spe_buf_destroy(io->_read_buffer);
  close(io->_fd);
  free(io);
}
