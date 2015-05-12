#ifndef __SPE_UNITTEST_H
#define __SPE_UNITTEST_H

#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(cond) do {                          \
  if (!(cond)) {                                        \
    printf("[TEST ERROR] %s:%d\n", __FILE__, __LINE__); \
  }                                                     \
} while(0)  

#define TEST_EQ(x, y) TEST_ASSERT(x == y)
#define TEST_LT(x, y) TEST_ASSERT(x > y)
#define TEST_ST(x, y) TEST_ASSERT(x < y)

#define TEST_STRING_EQ(x, y) TEST_ASSERT(!strcmp(x,y))

#endif
