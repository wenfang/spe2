#ifndef __SPE_SOCK_H 
#define __SPE_SOCK_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

extern int  
spe_sock_accept(int fd);

extern int  
spe_sock_accept_timeout(int fd, int timeout);

extern bool 
spe_sock_set_block(int fd, int block);

extern int  
spe_sock_tcp_server(const char* addr, int port);

extern int
spe_sock_udp_server(const char* addr, int port);

static inline int 
spe_sock_tcp_socket(void) {
  return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int 
spe_sock_udp_socket(void) {
  return socket(AF_INET, SOCK_DGRAM, 0);
}

static inline int 
spe_sock_close(int fd) {
  return close(fd);
}

#endif
