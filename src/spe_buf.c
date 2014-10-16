#include "spe_buf.h"
#include <stdio.h>

static bool
bufExpand(speBuf_t* buf, unsigned len) {
  if (buf->Data != buf->start) {
    memmove(buf->start, buf->Data, buf->Len);
    buf->Data[buf->Len] = 0;
  }

  unsigned newsize = 25 * len / 16;
  char* newstart = realloc(buf->start, newsize);
  if (!newstart) return false;

  buf->start  = newstart;
  buf->Data   = buf->start;
  buf->size   = newsize;
  return true;
}

/*
===================================================================================================
SpeBufCat
===================================================================================================
*/
bool
SpeBufCat(speBuf_t* buf, const char* src, unsigned len) {
  ASSERT(buf && src);
  if (buf->size - buf->Len < len + 1) {
    if (!bufExpand(buf, buf->size+len+1)) return false;
  }
  if (buf->size - buf->Len - (buf->Data - buf->start) < len + 1) {
    memmove(buf->start, buf->Data, buf->Len);
    buf->Data = buf->start;
  }
  memcpy(buf->Data + buf->Len, src, len);            
  buf->Len += len;
  buf->Data[buf->Len] = 0;
  return true;
}

/*
===================================================================================================
SpeBufCopy
===================================================================================================
*/
bool 
SpeBufCopy(speBuf_t* buf, const char* src, unsigned len) {
  ASSERT(buf && src);
  if (buf->size < len + 1) {
    if (!bufExpand(buf, len+1)) return false;
  }
  memcpy(buf->start, src, len);
  buf->Data           = buf->start;
  buf->Len            = len;
  buf->Data[buf->Len] = 0;
  return true;
}

/*
===================================================================================================
SpeBufLConsume
===================================================================================================
*/
int 
SpeBufLConsume(speBuf_t* buf, unsigned len) {
  ASSERT(buf);
  if (len >= buf->Len) {
    len = buf->Len;
    SpeBufClean(buf);
    return len; 
  }
  buf->Data           += len;
  buf->Len            -= len;
  buf->Data[buf->Len] = 0;
  return len;
}

/*
===================================================================================================
SpeBufRConsume
===================================================================================================
*/
int 
SpeBufRConsume(speBuf_t* buf, unsigned len) {
  ASSERT(buf);
  if (len >= buf->Len) {
    len = buf->Len;
    SpeBufClean(buf);
    return len;
  }
  buf->Len -= len;
  buf->Data[buf->Len] = 0;
  return len;
}

/*
===================================================================================================
SpeBufSearch
    search string in x
    return startpositon in x
            -1 for no found or error
===================================================================================================
*/
int 
SpeBufSearch(speBuf_t* buf, const char* key) {
  ASSERT(buf && key);
  if (!buf->Len) return -1;
  char* point = strstr(buf->Data, key);
  if (!point) return -1;
  return point - buf->Data;
}

/*
===================================================================================================
SpeBufLStrim
  strim string from left
===================================================================================================
*/
void 
SpeBufLStrim(speBuf_t* buf) {
  ASSERT(buf);
  int pos;
  for (pos = 0; pos < buf->Len; pos++) {
    if (buf->Data[pos] == '\t' || buf->Data[pos] == ' ' ||
        buf->Data[pos] == '\r' || buf->Data[pos] == '\n') continue;
    break;
  }
  if (pos) SpeBufLConsume(buf, pos);
}

/*
===================================================================================================
SpeBufRStrim
  strim string from right
===================================================================================================
*/
void 
SpeBufRStrim(speBuf_t* buf) {
  ASSERT(buf);
  int pos;
  for (pos = buf->Len-1; pos >= 0; pos--) {
    if (buf->Data[pos] == '\t' || buf->Data[pos] == ' ' ||
        buf->Data[pos] == '\r' || buf->Data[pos] == '\n') continue;
    break;
  }
  buf->Len            = pos + 1;
  buf->Data[buf->Len] = 0;
}

/*
===================================================================================================
SpeBufCmp
  compare two speBuf
===================================================================================================
*/
int 
SpeBufCmp(speBuf_t* buf1, speBuf_t* buf2) {
  ASSERT(buf1 && buf2);
  int minlen = (buf1->Len > buf2->Len) ? buf2->Len : buf1->Len;
  int ret = memcmp(buf1->Data, buf2->Data, minlen);
  if (ret) return ret;
  // length is equal  
  if (buf1->Len == buf2->Len) return 0;
  if (buf1->Len > buf2->Len) return 1;
  return -1;
}

/*
===================================================================================================
SpeBufToLower
  change buf to lower
===================================================================================================
*/
void
SpeBufToLower(speBuf_t* buf) {
  ASSERT(buf);
  for (int i=0; i<buf->Len; i++) {
    if (buf->Data[i]>='A' && buf->Data[i]<='Z') buf->Data[i] += 32;
  }
}

/*
===================================================================================================
SpeBufToUpper
  change buf to upper
===================================================================================================
*/
void
SpeBufToUpper(speBuf_t* buf) {
  ASSERT(buf);
  for (int i=0; i <buf->Len; i++) {
    if (buf->Data[i]>='a' && buf->Data[i]<='z') buf->Data[i] -= 32;
  }
}

/*
===================================================================================================
SpeBufRead
===================================================================================================
*/
int
SpeBufRead(int fd, speBuf_t* buf, unsigned len) {
  ASSERT(buf);
  if (buf->size < len + 1) {
    if (!bufExpand(buf, len+1)) return SPE_BUF_ERROR;
  }
  int res = read(fd, buf->start, buf->size - 1);
  if (res <= 0) return res;
  buf->Data           = buf->start;
  buf->Len            = res;
  buf->Data[buf->Len] = 0;
  return res;
}

/*
===================================================================================================
SpeBufReadAppend
===================================================================================================
*/
int
SpeBufReadAppend(int fd, speBuf_t* buf, unsigned len) {
  ASSERT(buf);
  if (buf->size - buf->Len < len + 1) {
    if (!bufExpand(buf, buf->size+len+1)) return SPE_BUF_ERROR;
  }
  if (buf->Data != buf->start) {
    memmove(buf->start, buf->Data, buf->Len);
  }
  int res = read(fd, buf->Data+buf->Len, buf->size - buf->Len - 1);
  if (res <= 0) return res;
  buf->Len            += res;
  buf->Data[buf->Len] = 0;
  return res;
}

/*
===================================================================================================
SpeBufSplit
===================================================================================================
*/
speBufList_t*
SpeBufSplit(speBuf_t* buf, const char* token) {
  ASSERT(buf && token);
  speBufList_t* bufList = SpeBufListCreate();
  if (!bufList) return NULL;
  // split from left to right
  int start = 0;
  while (start < buf->Len) {
    // split one by one
    char* point = strstr(buf->Data + start, token);
    if (!point) break;
    // add to string ignore null
    if (point != buf->Data + start) {
      SpeBufListAppend(bufList, buf->Data + start, point - buf->Data - start);
    }
    start = point - buf->Data + strlen(token);
  }
  if (buf->Len != start) {
    SpeBufListAppend(bufList, buf->Data + start, buf->Len - start);  
  }
  return bufList;
}

/*
===================================================================================================
SpeBufListAppend
===================================================================================================
*/
bool 
SpeBufListAppend(speBufList_t* bufList, char* src, unsigned len) {
  ASSERT(bufList && src);
  if (bufList->Len == bufList->size) {
    unsigned size = bufList->size;
    bufList->size = 16 + bufList->size;
    speBuf_t** newdata = realloc(bufList->Data, bufList->size*sizeof(speBuf_t*));
    if (!newdata) {
      bufList->size = size;
      return false;
    }
    bufList->Data = newdata;
    for (int i=bufList->Len; i<bufList->size; i++) bufList->Data[i] = NULL;
  }
  // copy string
  if (!bufList->Data[bufList->Len]) {
    bufList->Data[bufList->Len] = SpeBufCreate(0);
    if (!bufList->Data[bufList->Len]) return false;
  }
  SpeBufCopy(bufList->Data[bufList->Len], src, len);
  bufList->Len++;
  return true;
}
