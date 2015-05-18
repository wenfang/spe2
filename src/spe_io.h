#ifndef __SPE_IO_H
#define __SPE_IO_H

#include "spe_buf.h"

#define SPE_IO_CLOSED 0
#define SPE_IO_ERROR  -1

typedef struct spe_io_s {
  spe_buf_t*  _read_buffer;
  int         _fd;
  const char* _delim;
  unsigned    _rbytes;
  unsigned    _rtype:2;
  unsigned    _closed:1;
  unsigned    _error:1;
} spe_io_t;

extern int
spe_io_read(spe_io_t* io, spe_buf_t *buf);

extern int  
spe_io_readbytes(spe_io_t* io, unsigned len, spe_buf_t *buf);

extern int  
spe_io_readuntil(spe_io_t* io, const char* delim, spe_buf_t *buf);

extern int
spe_io_write(spe_io_t* io, spe_buf_t *buf);

extern spe_io_t*
spe_io_create(const char* fname);

extern spe_io_t*
spe_io_create_fd(int fd);

extern void
spe_io_destroy(spe_io_t* io);

#endif
