#ifndef __SPE_PROCESS_H
#define __SPE_PROCESS_H

#include "spe_handler.h"

extern int
SpeProcessSpawn(unsigned procNum);

extern void
SpeProcessEnableControl(speHandler_t handler);

#endif
