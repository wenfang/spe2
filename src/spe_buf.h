#ifndef __SPE_BUF_H
#define __SPE_BUF_H

#include "spe_util.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SPE_BUF_ERROR -2

typedef struct {
  char*     Data;
  char*     start;
  unsigned  Len;      
  unsigned  size;     
} speBuf_t __attribute__((aligned(sizeof(long))));

typedef struct {
  speBuf_t**  Data; 
  unsigned    Len;    
  unsigned    size;
} speBufs_t __attribute__((aligned(sizeof(long))));

extern bool 
SpeBufAppend(speBuf_t* buf, const char* src, unsigned len);

extern bool
SpeBufCopy(speBuf_t* buf, const char* src, unsigned len);

extern int
SpeBufLConsume(speBuf_t* buf, unsigned len);

extern int
SpeBufRConsume(speBuf_t* buf, unsigned len);

extern int
SpeBufSearch(speBuf_t* buf, const char* key);

extern void
SpeBufLStrim(speBuf_t* buf);

extern void
SpeBufRStrim(speBuf_t* buf);

extern int
SpeBufCmp(speBuf_t* buf1, speBuf_t* buf2);

extern void
SpeBufToLower(speBuf_t* buf);

extern int
SpeBufRead(int fd, speBuf_t* buf, unsigned len);

extern int
SpeBufReadAppend(int fd, speBuf_t* buf, unsigned len);

extern void
SpeBufToUpper(speBuf_t* buf);

static inline void 
SpeBufClean(speBuf_t* buf) {
  ASSERT(buf);
  if (!buf->start) return;
  buf->Data     = buf->start;
  buf->Len      = 0;
  buf->Data[0]  = 0;
}

static inline void 
SpeBufStrim(speBuf_t* buf) {
  ASSERT(buf);
  SpeBufLStrim(buf);
  SpeBufRStrim(buf);
}

static inline speBuf_t*
SpeBufCreate() {
  return calloc(1, sizeof(speBuf_t));
}

static inline void
SpeBufDestroy(speBuf_t* buf) {
  if (!buf) return;
  free(buf->start);
  free(buf);
}

static inline speBufs_t*
SpeBufsCreate() {
  return calloc(1, sizeof(speBufs_t));
}

static inline void
SpeBufsDestroy(speBufs_t* bufs) {
  if (!bufs) return;
  for (int i = 0; i < bufs->size; i++) {
    if (bufs->Data[i]) SpeBufDestroy(bufs->Data[i]);
  }
  free(bufs->Data);
  free(bufs);
}

static inline void 
SpeBufsClean(speBufs_t* bufs) {
  ASSERT(bufs);
  bufs->Len = 0;
}

extern speBufs_t* 
SpeBufSplit(speBuf_t* buf, const char* token);

extern bool 
SpeBufsAppend(speBufs_t* bufs, char* src, unsigned len);

#endif
