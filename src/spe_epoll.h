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
spe_epoll_enable(unsigned fd, unsigned mask, spe_task_t* task);

extern bool
spe_epoll_disable(unsigned fd, unsigned mask);

extern void
spe_epoll_process(int timeout);

extern spe_module_t spe_epoll_module;

#endif
