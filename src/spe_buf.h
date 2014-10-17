#ifndef __SPE_BUF_H
#define __SPE_BUF_H

#include "spe_util.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define DEFAULT_SIZE  128

#define SPE_BUF_ERROR -2

struct speBuf_s {
  char*     Data;
  char*     start;
  unsigned  Len;      
  unsigned  size;     
} __attribute__((aligned(sizeof(long))));
typedef struct speBuf_s speBuf_t;

struct speBufList_s {
  speBuf_t**  Data; 
  unsigned    Len;    
  unsigned    size;
} __attribute__((aligned(sizeof(long))));
typedef struct speBufList_s speBufList_t;

extern bool 
SpeBufCat(speBuf_t* buf, const char* src, unsigned len);

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
  if (!buf->Data) return;
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

static inline speBufList_t*
SpeBufListCreate() {
  return calloc(1, sizeof(speBufList_t));
}

static inline void
SpeBufListDestroy(speBufList_t* bufList) {
  if (!bufList) return;
  for (int i = 0; i < bufList->size; i++) {
    if (bufList->Data[i]) SpeBufDestroy(bufList->Data[i]);
  }
  free(bufList->Data);
  free(bufList);
}

static inline void 
SpeBufListClean(speBufList_t* bufList) {
  ASSERT(bufList);
  bufList->Len = 0;
}

extern speBufList_t* 
SpeBufSplit(speBuf_t* buf, const char* token);

extern bool 
SpeBufListAppend(speBufList_t* bufList, char* src, unsigned len);

#endif
