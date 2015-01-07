#ifndef __SPE_PROCESS_H
#define __SPE_PROCESS_H

#include "spe_handler.h"

extern int
SpeProcessFork();

extern void
SpeProcessEnableControl(speHandler_t handler);

extern void
SpeProcessDisableControl();

#endif
