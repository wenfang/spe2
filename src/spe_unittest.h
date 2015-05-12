#ifndef __SPE_UNITTEST_H
#define __SPE_UNITTEST_H

#include <stdio.h>
#include <string.h>

#define TEST_EQ(x, y)   do {                            \
  if (x != y) {                                         \
    printf("[TEST ERROR] %s:%d\n", __FILE__, __LINE__); \
  }                                                     \
} while(0)  

#define TEST_STRING_EQ(x, y) do {                       \
  if (strcmp(x, y)) {                                   \
    printf("[TEST ERROR] %s:%d\n", __FILE__, __LINE__); \
  }                                                     \
} while(0)  

#endif
