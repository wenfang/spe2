#ifndef __SPE_IO_H
#define __SPE_IO_H

#include "spe_buf.h"

typedef struct spe_io_s {
  spe_buf_t*   read_buffer;
  unsigned    fd;
  const char* delim;
  unsigned    rbytes;
  unsigned    rtype:2;
  unsigned    closed:1;
  unsigned    error:1;
} spe_io_t;

extern int  
spe_io_read(spe_io_t* io, unsigned len, spe_buf_t *buf);

extern int  
spe_io_read_until(spe_io_t* io, const char* delim, spe_buf_t *buf);

extern int
spe_io_write(spe_io_t* io, spe_buf_t *buf);

extern spe_io_t*
spe_io_create(const char* fname);

extern spe_io_t*
spe_io_create_fd(int fd);

extern void
spe_io_destroy(spe_io_t* io);

#endif
