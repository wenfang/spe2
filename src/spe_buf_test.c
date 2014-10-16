#include "spe_buf.h"
#include <stdio.h>
#include <string.h>

void TestCopy() {
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, "test", strlen("test"));
  printf("TestCopy: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufDestroy(buf);
}

void TestCat() {
  speBuf_t* buf = SpeBufCreate();
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  SpeBufCat(buf, "test", strlen("test"));
  printf("TestCat: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufDestroy(buf);
}

void TestConsume() {
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, "test", strlen("test"));
  SpeBufLConsume(buf, 1);
  printf("TestLConsume: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufRConsume(buf, 1);
  printf("TestRConsume: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufDestroy(buf);
}

void TestUpperLower() {
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, "test", strlen("test"));
  SpeBufToUpper(buf);
  printf("TestToUpper: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufToLower(buf);
  printf("TestToLower: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufDestroy(buf);
}

void TestClean() {
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, "test", strlen("test"));
  SpeBufClean(buf);
  printf("TestClean: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufDestroy(buf);
}

void TestStrim() {
  char *str = "   this is a test string   ";
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, str, strlen(str));
  SpeBufLStrim(buf);
  printf("TestLStrim: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufRStrim(buf);
  printf("TestRStrim: %s, %d, %d\n", buf->Data, buf->Len, buf->size);
  SpeBufDestroy(buf);
}

void TestSearch() {
  char *str = "   this is a test string   ";
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, str, strlen(str));
  int pos = SpeBufSearch(buf, "test");
  printf("TestSearch: %s, pos %d\n", buf->Data, pos);
  SpeBufDestroy(buf);
}

void TestCmp() {
  char *str1 = "test";
  char *str2 = "tEst";
  speBuf_t* buf1 = SpeBufCreate();
  SpeBufCopy(buf1, str1, strlen(str1));
  speBuf_t* buf2 = SpeBufCreate();
  SpeBufCopy(buf2, str2, strlen(str2));
  int rc = SpeBufCmp(buf1, buf2);
  printf("TestCmp: %d\n", rc);
  SpeBufDestroy(buf1);
  SpeBufDestroy(buf2);
}

void TestSplit() {
  char *str = " this is a test string ";
  speBuf_t* buf = SpeBufCreate();
  SpeBufCopy(buf, str, strlen(str));
  speBufList_t* bufList = SpeBufSplit(buf, " ");
  for (int i=0; i<bufList->Len; i++) {
    printf("TestSplit %d, %s, %d, %d\n", i, bufList->Data[i]->Data, bufList->Data[i]->Len, bufList->Data[i]->size);
  }
  SpeBufListDestroy(bufList);
  SpeBufDestroy(buf);
}

void TestBufList() {
  speBufList_t* bufList = SpeBufListCreate();
  SpeBufListAppend(bufList, "test1", strlen("test1"));
  SpeBufListAppend(bufList, "test1", strlen("test1"));
  SpeBufListAppend(bufList, "test1", strlen("test1"));
  SpeBufListAppend(bufList, "test1", strlen("test1"));
  for (int i=0; i<bufList->Len; i++) {
    printf("TestSplit %d, %s, %d, %d\n", i, bufList->Data[i]->Data, bufList->Data[i]->Len, bufList->Data[i]->size);
  }
  SpeBufListDestroy(bufList);
}

int main() {
  TestCopy();
  TestCat();
  TestConsume();
  TestUpperLower();
  TestClean();
  TestStrim();
  TestSearch();
  TestCmp();
  TestSplit();
  TestBufList();
  return 1;
}
