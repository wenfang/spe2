#include "spe_buf.h"
#include "spe_unittest.h"
#include <stdio.h>
#include <string.h>

void TestCopy() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  TEST_STRING_EQ("test", buf->data);
  TEST_EQ(buf->len, 4);
  spe_buf_copy(buf, "abcdefg", strlen("abcdefg"));
  TEST_STRING_EQ("abcdefg", buf->data);
  TEST_EQ(buf->len, 7);
  spe_buf_destroy(buf);
}

void TestCat() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  spe_buf_append(buf, "test", strlen("test"));
  printf("TestCat: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_destroy(buf);
}

void TestConsume() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  spe_buf_lconsume(buf, 1);
  printf("TestLConsume: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_rconsume(buf, 1);
  printf("TestRConsume: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_destroy(buf);
}

void TestUpperLower() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  spe_buf_to_upper(buf);
  printf("TestToUpper: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_to_lower(buf);
  printf("TestToLower: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_destroy(buf);
}

void TestClean() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  spe_buf_clean(buf);
  printf("TestClean: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_destroy(buf);
}

void TestStrim() {
  char *str = "   this is a test string   ";
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, str, strlen(str));
  spe_buf_lstrim(buf, " \n\r");
  printf("TestLStrim: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_rstrim(buf, " \n\r");
  printf("TestRStrim: %s, %d, %d\n", buf->data, buf->len, buf->_size);
  spe_buf_destroy(buf);
}

void TestSearch() {
  char *str = "   this is a test string   ";
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, str, strlen(str));
  int pos = spe_buf_search(buf, "test");
  printf("TestSearch: %s, pos %d\n", buf->data, pos);
  spe_buf_destroy(buf);
}

void TestCmp() {
  char *str1 = "test";
  char *str2 = "tEst";
  spe_buf_t* buf1 = spe_buf_create();
  spe_buf_copy(buf1, str1, strlen(str1));
  spe_buf_t* buf2 = spe_buf_create();
  spe_buf_copy(buf2, str2, strlen(str2));
  int rc = spe_buf_cmp(buf1, buf2);
  printf("TestCmp: %d\n", rc);
  spe_buf_destroy(buf1);
  spe_buf_destroy(buf2);
}

void TestSplit() {
  char *str = " this is a test string ";
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, str, strlen(str));
  spe_bufs_t* bufList = spe_buf_split(buf, " ");
  for (int i=0; i<bufList->len; i++) {
    printf("TestSplit %d, %s, %d, %d\n", i, bufList->data[i]->data, bufList->data[i]->len, bufList->data[i]->_size);
  }
  spe_bufs_destroy(bufList);
  spe_buf_destroy(buf);
}

void TestBufList() {
  spe_bufs_t* bufList = spe_bufs_create();
  spe_bufs_append(bufList, "test1", strlen("test1"));
  spe_bufs_append(bufList, "test1", strlen("test1"));
  spe_bufs_append(bufList, "test1", strlen("test1"));
  spe_bufs_append(bufList, "test1", strlen("test1"));
  for (int i=0; i<bufList->len; i++) {
    printf("TestSplit %d, %s, %d, %d\n", i, bufList->data[i]->data, bufList->data[i]->len, bufList->data[i]->_size);
  }
  spe_bufs_destroy(bufList);
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
