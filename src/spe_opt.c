#include "spe_io.h"
#include "spe_opt.h"
#include "spe_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#define SEC_MAXLEN  16
#define KEY_MAXLEN  16
#define VAL_MAXLEN  128

struct SpeOpt_s {
	char              sec[SEC_MAXLEN];
 	char              key[KEY_MAXLEN];
	char              val[VAL_MAXLEN];      // string value
	struct list_head  node;
};
typedef struct SpeOpt_s SpeOpt_t;

static LIST_HEAD(options);

/*
===================================================================================================
SpeOptSet
===================================================================================================
*/
static bool 
SpeOptSet(char* sec, char* key, char* val) {
  SpeOpt_t* opt = calloc (1, sizeof(SpeOpt_t));
  if (!opt) return false;
  // set section and key 
  strncpy(opt->sec, sec, SEC_MAXLEN);
  strncpy(opt->key, key, KEY_MAXLEN);
  strncpy(opt->val, val, VAL_MAXLEN);
  // add to options list
  INIT_LIST_HEAD(&opt->node);
  list_add_tail(&opt->node, &options);
  return true;
}

/*
===================================================================================================
SpeOptInt
===================================================================================================
*/
int
SpeOptInt(char* section, char* key, int defaultValue) {
  if (!key) return defaultValue;
  if (!section) section = "global";
  SpeOpt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(section, entry->sec) && !strcmp(key, entry->key)) {
      return atoi(entry->val);
    }
  }
  return defaultValue;
}

/*
===================================================================================================
SpeOptString
===================================================================================================
*/
const char* 
SpeOptString(char* section, char* key, const char* defaultValue) {
  if (!key) return defaultValue;
  if (!section) section = "global";
  SpeOpt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(section, entry->sec) && !strcmp(key, entry->key)) {
      return entry->val;
    }
  }
  return defaultValue;
}

/*
===================================================================================================
SpeOptCreate
===================================================================================================
*/
bool 
SpeOptCreate(const char* configFile) {
  char sec[SEC_MAXLEN];
  char key[KEY_MAXLEN];
  char val[VAL_MAXLEN];

  speIO_t* io = SpeIOCreate(configFile);
  if (!io) return false;
  speBuf_t* line = SpeBufCreate();
  if (!line) return false;
  // set default section
  strcpy(sec, "global");
  while (1) {
    // get one line from file
    SpeIOReaduntil(io, "\n");
    if (io->Closed || io->Error) break;
    SpeBufCopy(line, io->ReadBuffer->Data, io->RLen);
    SpeBufStrim(line);
    SpeBufLConsume(io->ReadBuffer, io->RLen);
    // ignore empty and comment line
    if (line->Len == 0 || line->Data[0] == '#') continue;
    // section line, get section
    if (line->Data[0] == '[' && line->Data[line->Len-1] == ']') {
      SpeBufLConsume(line, 1);
      SpeBufRConsume(line, 1);
      SpeBufStrim(line);
      // section can't be null
      if (line->Len == 0) {
        SpeBufDestroy(line);
        SpeIODestroy(io);
        return false;
      }
      strncpy(sec, line->Data, SEC_MAXLEN);
      sec[SEC_MAXLEN-1] = 0;
      continue;
    }
    // split key and value
    speBufList_t* bufList = SpeBufSplit(line, "=");
    if (bufList->Len != 2) {
      SpeBufListDestroy(bufList);
      SpeBufDestroy(line);
      SpeIODestroy(io);
      return false;
    }
    SpeBufStrim(bufList->Data[0]);
    strncpy(key, bufList->Data[0]->Data, KEY_MAXLEN);
    key[KEY_MAXLEN-1] = 0;

    SpeBufStrim(bufList->Data[1]);
    strncpy(val, bufList->Data[1]->Data, VAL_MAXLEN);
    val[VAL_MAXLEN-1] = 0;
    SpeBufListDestroy(bufList);
    // set option value
    SpeOptSet(sec, key, val);
  }
  SpeBufDestroy(line);
  SpeIODestroy(io);
  return true;
}

/*
===================================================================================================
SpeOptDestroy
===================================================================================================
*/
void
SpeOptDestroy() {
  SpeOpt_t *entry, *tmp;
  list_for_each_entry_safe(entry, tmp, &options, node) {
    list_del_init(&entry->node);
    free(entry);
  }
}
