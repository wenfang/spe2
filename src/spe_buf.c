#include "spe_buf.h"
#include <stdio.h>

static bool
buf_expand(spe_buf_t* buf, unsigned len) {
  if (buf->data != buf->_start) {
    memmove(buf->_start, buf->data, buf->len);
    buf->data           = buf->_start;
    buf->data[buf->len] = 0;
  }

  unsigned new_size = 25 * len / 16;
  char* new_start = realloc(buf->_start, new_size);
  if (new_start == NULL) return false;

  buf->_start = new_start;
  buf->_size  = new_size;
  buf->data   = buf->_start;
  return true;
}

/*
===================================================================================================
spe_buf_append
===================================================================================================
*/
bool
spe_buf_append(spe_buf_t* buf, const char* src, unsigned len) {
  ASSERT(buf && src);
  if (buf->_size - buf->len < len + 1) {
    if (!buf_expand(buf, buf->_size + len + 1)) return false;
  }
  if (buf->_size - buf->len - (buf->data - buf->_start) < len + 1) {
    memmove(buf->_start, buf->data, buf->len);
    buf->data           = buf->_start;
    buf->data[buf->len] = 0;
  }
  memcpy(buf->data + buf->len, src, len);            
  buf->len += len;
  buf->data[buf->len] = 0;
  return true;
}

/*
===================================================================================================
spe_buf_copy
===================================================================================================
*/
bool 
spe_buf_copy(spe_buf_t* buf, const char* src, unsigned len) {
  ASSERT(buf && src);
  if (buf->_size < len + 1) {
    if (!buf_expand(buf, len+1)) return false;
  }
  memcpy(buf->_start, src, len);
  buf->data           = buf->_start;
  buf->len            = len;
  buf->data[buf->len] = 0;
  return true;
}

/*
===================================================================================================
spe_buf_lconsume
===================================================================================================
*/
void
spe_buf_lconsume(spe_buf_t* buf, unsigned len) {
  ASSERT(buf);
  if (len > buf->len) len = buf->len;
  buf->data           += len;
  buf->len            -= len;
  buf->data[buf->len] = 0;
}

/*
===================================================================================================
spe_buf_rconsume
===================================================================================================
*/
void
spe_buf_rconsume(spe_buf_t* buf, unsigned len) {
  ASSERT(buf);
  if (len >= buf->len) len = buf->len;
  buf->len            -= len;
  buf->data[buf->len] = 0;
}

/*
===================================================================================================
spe_buf_search
    search string in x
    return _startpositon in x
            -1 for no found or error
===================================================================================================
*/
int 
spe_buf_search(spe_buf_t* buf, const char* key) {
  ASSERT(buf && key);
  if (!buf->len) return -1;
  char* point = strstr(buf->data, key);
  if (!point) return -1;
  return point - buf->data;
}

/*
===================================================================================================
spe_buf_lstrim
  strim string from left
===================================================================================================
*/
void 
spe_buf_lstrim(spe_buf_t* buf, char* token) {
  ASSERT(buf && token);
  int pos;
  for (pos = 0; pos < buf->len; pos++) {
    if (!strchr(token, buf->data[pos])) break;
  }
  if (pos) spe_buf_lconsume(buf, pos);
}

/*
===================================================================================================
spe_buf_rstrim
  strim string from right
===================================================================================================
*/
void 
spe_buf_rstrim(spe_buf_t* buf, char* token) {
  ASSERT(buf && token);
  int pos;
  for (pos = buf->len-1; pos >= 0; pos--) {
    if (!strchr(token, buf->data[pos])) break;
  }
  buf->len            = pos + 1;
  buf->data[buf->len] = 0;
}

/*
===================================================================================================
spe_buf_cmp
  compare two speBuf
===================================================================================================
*/
int 
spe_buf_cmp(spe_buf_t* buf1, spe_buf_t* buf2) {
  ASSERT(buf1 && buf2);
  int minlen = (buf1->len > buf2->len) ? buf2->len : buf1->len;
  int ret = memcmp(buf1->data, buf2->data, minlen);
  if (ret) return ret;
  // length is equal  
  if (buf1->len == buf2->len) return 0;
  if (buf1->len > buf2->len) return 1;
  return -1;
}

/*
===================================================================================================
spe_buf_to_lower
  change buf to lower
===================================================================================================
*/
void
spe_buf_to_lower(spe_buf_t* buf) {
  ASSERT(buf);
  for (int i=0; i<buf->len; i++) {
    if (buf->data[i]>='A' && buf->data[i]<='Z') buf->data[i] += 32;
  }
}

/*
===================================================================================================
spe_buf_to_upper
  change buf to upper
===================================================================================================
*/
void
spe_buf_to_upper(spe_buf_t* buf) {
  ASSERT(buf);
  for (int i=0; i <buf->len; i++) {
    if (buf->data[i]>='a' && buf->data[i]<='z') buf->data[i] -= 32;
  }
}

/*
===================================================================================================
spe_buf_read_fd_copy
===================================================================================================
*/
int
spe_buf_read_fd_copy(int fd, unsigned len, spe_buf_t* buf) {
  ASSERT(buf);
  if (buf->_size < len + 1) {
    if (!buf_expand(buf, len+1)) return SPE_BUF_ERROR;
  }
  int res = read(fd, buf->_start, buf->_size - 1);
  if (res <= 0) return res;
  buf->data           = buf->_start;
  buf->len            = res;
  buf->data[buf->len] = 0;
  return res;
}

/*
===================================================================================================
spe_buf_read_fd_append
===================================================================================================
*/
int
spe_buf_read_fd_append(int fd, unsigned len, spe_buf_t* buf) {
  ASSERT(buf);
  if (buf->_size - buf->len < len + 1) {
    if (!buf_expand(buf, buf->_size+len+1)) return SPE_BUF_ERROR;
  }
  if (buf->data != buf->_start) {
    memmove(buf->_start, buf->data, buf->len);
  }
  int res = read(fd, buf->data+buf->len, buf->_size - buf->len - 1);
  if (res <= 0) return res;
  buf->len            += res;
  buf->data[buf->len] = 0;
  return res;
}

/*
===================================================================================================
spe_buf_split
===================================================================================================
*/
spe_bufs_t*
spe_buf_split(spe_buf_t* buf, const char* token) {
  ASSERT(buf && token);
  spe_bufs_t* bufs = spe_bufs_create();
  if (!bufs) return NULL;
  // split from left to right
  int start = 0;
  while (start < buf->len) {
    // split one by one
    char* point = strstr(buf->data + start, token);
    if (!point) break;
    // add to string ignore null
    if (point != buf->data + start) {
      spe_bufs_append(bufs, buf->data + start, point - buf->data - start);
    }
    start = point - buf->data + strlen(token);
  }
  if (buf->len != start) {
    spe_bufs_append(bufs, buf->data + start, buf->len - start);  
  }
  return bufs;
}

/*
===================================================================================================
spe_bufs_append
===================================================================================================
*/
bool 
spe_bufs_append(spe_bufs_t* bufList, char* src, unsigned len) {
  ASSERT(bufList && src);
  if (bufList->len == bufList->_size) {
    unsigned _size = bufList->_size;
    bufList->_size = 16 + bufList->_size;
    spe_buf_t** newdata = realloc(bufList->data, bufList->_size*sizeof(spe_buf_t*));
    if (!newdata) {
      bufList->_size = _size;
      return false;
    }
    bufList->data = newdata;
    for (int i=bufList->len; i<bufList->_size; i++) bufList->data[i] = NULL;
  }
  // copy string
  if (!bufList->data[bufList->len]) {
    bufList->data[bufList->len] = spe_buf_create(0);
    if (!bufList->data[bufList->len]) return false;
  }
  spe_buf_copy(bufList->data[bufList->len], src, len);
  bufList->len++;
  return true;
}
