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
  TEST_STRING_EQ("testtesttest", buf->data);
  TEST_EQ(buf->len, 12);
  spe_buf_destroy(buf);
}

void TestConsume() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  spe_buf_lconsume(buf, 1);
  TEST_STRING_EQ("est", buf->data);
  TEST_EQ(buf->len, 3);
  spe_buf_rconsume(buf, 1);
  TEST_STRING_EQ("es", buf->data);
  TEST_EQ(buf->len, 2);
  spe_buf_destroy(buf);
}

void TestUpperLower() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  spe_buf_to_upper(buf);
  TEST_STRING_EQ("TEST", buf->data);
  TEST_EQ(buf->len, 4);
  spe_buf_to_lower(buf);
  TEST_STRING_EQ("test", buf->data);
  TEST_EQ(buf->len, 4);
  spe_buf_destroy(buf);
}

void TestClean() {
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, "test", strlen("test"));
  spe_buf_clean(buf);
  TEST_EQ(buf->data[0], 0);
  TEST_EQ(buf->len, 0);
  spe_buf_destroy(buf);
}

void TestStrim() {
  char *str = "   this is a test string   ";
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, str, strlen(str));
  spe_buf_lstrim(buf, " \n\r");
  TEST_STRING_EQ("this is a test string   ", buf->data);
  spe_buf_rstrim(buf, " \n\r");
  TEST_STRING_EQ("this is a test string", buf->data);
  spe_buf_destroy(buf);
}

void TestSearch() {
  char *str = "   this is a test string   ";
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, str, strlen(str));
  int pos = spe_buf_search(buf, "test");
  TEST_EQ(pos, 13);
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
  TEST_LT(rc, 0);
  spe_buf_destroy(buf1);
  spe_buf_destroy(buf2);
}

void TestSplit() {
  char *str = " this is a test string ";
  spe_buf_t* buf = spe_buf_create();
  spe_buf_copy(buf, str, strlen(str));
  spe_bufs_t* bufs = spe_buf_split(buf, " ");
  TEST_STRING_EQ(bufs->data[0]->data, "this");
  TEST_STRING_EQ(bufs->data[1]->data, "is");
  TEST_STRING_EQ(bufs->data[2]->data, "a");
  TEST_STRING_EQ(bufs->data[3]->data, "test");
  TEST_STRING_EQ(bufs->data[4]->data, "string");
  spe_bufs_destroy(bufs);
  spe_buf_destroy(buf);
}

void TestBufs() {
  spe_bufs_t* bufs = spe_bufs_create();
  spe_bufs_append(bufs, "test1", strlen("test1"));
  spe_bufs_append(bufs, "test1", strlen("test1"));
  spe_bufs_append(bufs, "test1", strlen("test1"));
  spe_bufs_append(bufs, "test1", strlen("test1"));
  for (int i=0; i<bufs->len; i++) {
    TEST_STRING_EQ(bufs->data[i]->data, "test1");
    TEST_EQ(bufs->data[i]->len, 5);
  }
  spe_bufs_destroy(bufs);
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
  TestBufs();
  return 1;
}
