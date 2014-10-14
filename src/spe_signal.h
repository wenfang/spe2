#ifndef __SPE_SIGNAL_H
#define __SPE_SIGNAL_H

#include "spe_module.h"

typedef void SpeSignal_Handler(int sig);

extern void 
SpeSignalRegister(int sig, SpeSignal_Handler handler);

extern void 
SpeSignalProcess();

extern speModule_t speSignalModule;

#endif
