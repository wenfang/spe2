#ifndef __SPE_EPOLL_H
#define __SPE_EPOLL_H

#include "spe_module.h"
#include "spe_task.h"
#include <stdbool.h>

#define SPE_EPOLL_NONE   0
#define SPE_EPOLL_LISTEN 1
#define SPE_EPOLL_READ   1
#define SPE_EPOLL_WRITE  2

extern bool
SpeEpollEnable(unsigned fd, unsigned mask, speTask_t* task);

extern bool
SpeEpollDisable(unsigned fd, unsigned mask);

extern void
SpeEpollProcess(int timeout);

extern speModule_t speEpollModule;

#endif
