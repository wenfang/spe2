#include "spe_map.h"
#include <stdio.h>

void TestCreate() {
  speMap_t* map = SpeMapCreate(17, NULL);
  SpeMapSet(map, "test1", NULL);
  SpeMapSet(map, "test2", NULL);
  SpeMapSet(map, "test3", NULL);
  SpeMapSet(map, "test1", NULL);
  fprintf(stdout, "Map Len: %d\n", map->Len);
  SpeMapDel(map, "test2");
  fprintf(stdout, "Map Len: %d\n", map->Len);
  SpeMapDestroy(map);
}

int main() {
  TestCreate();
}
