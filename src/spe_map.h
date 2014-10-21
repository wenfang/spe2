#ifndef __SPE_MAP_H 
#define __SPE_MAP_H

#include "spe_list.h"
#include <stdbool.h>

#define SPE_MAP_KEYLEN 16

#define SPE_MAP_OK        0
#define SPE_MAP_ERROR     -1
#define SPE_MAP_CONFLICT  -2

typedef void (*SpeMapHandler)(void*);

typedef struct {
  unsigned          Len;
  unsigned          size;           // element size
  SpeMapHandler     handler;
  struct list_head  listHead;       // list Head
  struct hlist_head hashHead[0];    // element of mjitem
} speMap_t;

typedef struct {
  char              key[SPE_MAP_KEYLEN];
  void*             obj;
  struct list_head  listNode;
  struct hlist_node hashNode;
} speMapItem_t;

extern speMap_t* 
SpeMapCreate(unsigned size, SpeMapHandler handler);

extern void 
SpeMapDestroy(speMap_t* map);

extern void
SpeMapClean(speMap_t* map);

extern void* 
SpeMapGet(speMap_t* map, const char* key);

extern int 
SpeMapSet(speMap_t* map, const char* key, void* obj);

extern bool   
SpeMapDel(speMap_t* map, const char* key);

extern speMapItem_t*
SpeMapNext(speMap_t* map, speMapItem_t* item);

#endif
