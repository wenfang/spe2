#ifndef __SPE_BUF_H
#define __SPE_BUF_H

#include "spe_util.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SPE_BUF_ERROR   -2
#define SPE_BUF_EAGAIN  -3

typedef struct spe_buf_s {
  char*     data;
  char*     _start;
  unsigned  len;      
  unsigned  _size;     
} spe_buf_t __attribute__((aligned(sizeof(long))));

typedef struct spe_bufs_s {
  spe_buf_t** data; 
  unsigned    len;    
  unsigned    _size;
} spe_bufs_t __attribute__((aligned(sizeof(long))));

extern bool 
spe_buf_append(spe_buf_t* buf, const char* src, unsigned len);

extern bool
spe_buf_copy(spe_buf_t* buf, const char* src, unsigned len);

extern void
spe_buf_lconsume(spe_buf_t* buf, unsigned len);

extern void
spe_buf_rconsume(spe_buf_t* buf, unsigned len);

extern int
spe_buf_search(spe_buf_t* buf, const char* key);

extern void
spe_buf_lstrim(spe_buf_t* buf, char* token);

extern void
spe_buf_rstrim(spe_buf_t* buf, char* token);

extern int
spe_buf_cmp(spe_buf_t* buf1, spe_buf_t* buf2);

extern void
spe_buf_to_lower(spe_buf_t* buf);

extern void
spe_buf_to_upper(spe_buf_t* buf);

extern int
spe_buf_read_fd_append(int fd, unsigned len, spe_buf_t* buf);

extern int
spe_buf_read_fd_copy(int fd, unsigned len, spe_buf_t* buf);

static inline void 
spe_buf_clean(spe_buf_t* buf) {
  ASSERT(buf);
  buf->data = buf->_start;
  buf->len  = 0;
  if (buf->_size) buf->data[0] = 0;
}

static inline void 
spe_buf_strim(spe_buf_t* buf, char* token) {
  ASSERT(buf && token);
  spe_buf_lstrim(buf, token);
  spe_buf_rstrim(buf, token);
}

static inline spe_buf_t*
spe_buf_create() {
  return calloc(1, sizeof(spe_buf_t));
}

static inline void
spe_buf_destroy(spe_buf_t* buf) {
  if (buf == NULL) return;
  free(buf->_start);
  free(buf);
}

static inline spe_bufs_t*
spe_bufs_create() {
  return calloc(1, sizeof(spe_bufs_t));
}

static inline void
spe_bufs_destroy(spe_bufs_t* bufs) {
  if (!bufs) return;
  for (int i = 0; i < bufs->_size; i++) {
    if (bufs->data[i]) spe_buf_destroy(bufs->data[i]);
  }
  free(bufs->data);
  free(bufs);
}

static inline void 
spe_bufs_clean(spe_bufs_t* bufs) {
  ASSERT(bufs);
  bufs->len = 0;
}

extern spe_bufs_t* 
spe_buf_split(spe_buf_t* buf, const char* token);

extern bool 
spe_bufs_append(spe_bufs_t* bufs, char* src, unsigned len);

#endif
