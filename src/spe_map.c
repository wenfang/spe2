#include "spe_map.h"
#include "spe_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
===================================================================================================
speMapItemCreate
===================================================================================================
*/
static speMapItem_t*
speMapItemCreate(const char* key, void* obj) {
  speMapItem_t* item = calloc(1, sizeof(speMapItem_t));
  if (!item) return NULL;
  strncpy(item->key, key, SPE_MAP_KEYLEN-1);
  item->key[SPE_MAP_KEYLEN-1] = 0;
  item->obj               = obj;
  INIT_LIST_HEAD(&item->listNode);
  INIT_HLIST_NODE(&item->hashNode);
  return item;
}

/*
===================================================================================================
spe_map_item_destroy
===================================================================================================
*/
static inline void 
speMapItemDestroy(speMapItem_t* item, SpeMapHandler handler) {
  ASSERT(item);
  list_del(&item->listNode);
  hlist_del(&item->hashNode);
  if (handler) handler(item->obj);
  free(item);
}

/*
===================================================================================================
 gen hash value, copy from redis
===================================================================================================
*/
static unsigned int genhashvalue(const void* key, int len)
{
  uint32_t seed = 5381;
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  /* Initialize the hash to a 'random' value */
  uint32_t h = seed ^ len;

  /* Mix 4 bytes at a time into the hash */
  const unsigned char *data = (const unsigned char *)key;

  while (len >= 4) {
    uint32_t k = *(uint32_t*)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  /* Handle the last few bytes of the input array  */
  switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
  };

  /* Do a few final mixes of the hash to ensure the last few 
     bytes are well-incorporated. */
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return (unsigned int)h;
}

/*
===================================================================================================
speMapSearch
===================================================================================================
*/
static speMapItem_t*
speMapSearch(speMap_t* map, const char* key) {
  unsigned int hashvalue = genhashvalue((void*)key, strlen(key));
  unsigned int index = hashvalue % map->size;
  // search entry
  speMapItem_t* item = NULL;
  struct hlist_node* entry;
  hlist_for_each_entry(item, entry, &map->hashHead[index], hashNode) { 
    if (strcmp(item->key, key) == 0) return item;
  }
  return NULL;
}

/*
===================================================================================================
SpeMapGet
===================================================================================================
*/
void*
SpeMapGet(speMap_t* map, const char* key) {
  if (!map || !key) return NULL;
  speMapItem_t* item = speMapSearch(map, key);
  if (!item) return NULL;
  return item->obj;
}

/*
===================================================================================================
SpeMapSet
===================================================================================================
*/
int 
SpeMapSet(speMap_t* map, const char* key, void* obj) {
  if (!map || !key) return SPE_MAP_ERROR;
  unsigned int hashvalue = genhashvalue((void*)key, strlen(key));
  unsigned int index = hashvalue % map->size;
  speMapItem_t* item = speMapSearch(map, key);
  if (item) return SPE_MAP_CONFLICT;
  item = speMapItemCreate(key, obj);
  if (!item) return SPE_MAP_ERROR;
  // add to list and elem list
  list_add_tail(&item->listNode, &map->listHead);
  hlist_add_head(&item->hashNode, &map->hashHead[index]);
  map->Len++;
  return SPE_MAP_OK;
}

/*
===================================================================================================
SpeMapDel
===================================================================================================
*/
bool 
SpeMapDel(speMap_t* map, const char* key) {
  if (!map || !key) return false;
  speMapItem_t* item = speMapSearch(map, key);
  if (!item) return false;
  speMapItemDestroy(item, map->handler);
  map->Len--;
  return true;
}

/*
===================================================================================================
SpeMapClean
===================================================================================================
*/
void
SpeMapClean(speMap_t* map) {
  if (!map) return;
	speMapItem_t* item;
  struct hlist_node* entry;
  struct hlist_node* tmp;
  for (int i=0; i<map->size; i++) {
    hlist_for_each_entry_safe(item, entry, tmp, &map->hashHead[i], hashNode) { 
		  speMapItemDestroy(item, map->handler);
	  }
  }
  map->Len = 0;
}

/*
===================================================================================================
SpeMapCreate
===================================================================================================
*/
speMap_t*
SpeMapCreate(unsigned size, SpeMapHandler handler) {
  ASSERT(size);
  speMap_t* map = calloc(1, sizeof(speMap_t) + size * sizeof(struct hlist_head));
  if (!map) return NULL;
  map->size     = size;
  map->handler  = handler;
  INIT_LIST_HEAD(&map->listHead);
  for (int i=0; i<size; i++) {
    INIT_HLIST_HEAD(&map->hashHead[i]);
  }
  return map;
}

/*
===================================================================================================
SpeMapDestroy
===================================================================================================
*/
void
SpeMapDestroy(speMap_t* map) {
  if (!map) return;
  SpeMapClean(map);
  free(map);
}

/*
===================================================================================================
SpeMapNext
===================================================================================================
*/
speMapItem_t*
SpeMapNext(speMap_t* map, speMapItem_t* item) {
  ASSERT(map);
  if (!item) {
    if (list_empty(&map->listHead)) return NULL;
    item = list_first_entry(&map->listHead, speMapItem_t, listNode);
    return item;
  }
  list_for_each_entry_continue(item, &map->listHead, listNode) break;
  if (&item->listNode == &map->listHead) return NULL;
  return item;
}
