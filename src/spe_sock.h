#ifndef __SPE_SOCK_H 
#define __SPE_SOCK_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

extern int  
SpeSockAccept(int fd);

extern int  
SpeSockAcceptTimeout(int fd, int timeout);

extern bool 
SpeSockSetBlock(int fd, int block);

extern int  
SpeSockTcpServer(const char* addr, int port);

static inline int 
SpeSockTcpSocket() {
  return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int 
SpeSockUdpSocket() {
  return socket(AF_INET, SOCK_DGRAM, 0);
}

static inline int 
SpeSockClose(int fd) {
  return close(fd);
}

#endif
