#ifndef __SPE_SIGNAL_H
#define __SPE_SIGNAL_H

#include "spe_module.h"

typedef void spe_signal_handler(int sig);

extern void 
spe_signal_register(int sig, spe_signal_handler handler);

extern void 
spe_signal_process(void);

extern spe_module_t spe_signal_module;

#endif
