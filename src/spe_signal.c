#include "spe_signal.h"
#include <string.h>
#include <signal.h>

#define MAX_SIGNAL 256

struct speSignal_s {
  unsigned            count;
  SpeSignal_Handler*  handler;
};
typedef struct speSignal_s speSignal_t;

static int          signalQueueLen;
static int          signalQueue[MAX_SIGNAL];
static speSignal_t  signalState[MAX_SIGNAL];
static sigset_t     blockedSig;

/*
===================================================================================================
signalInit
===================================================================================================
*/
static void 
signalInit(speCycle_t *cycle) {
  signalQueueLen = 0;
  memset(signalQueue, 0, sizeof(signalQueue));
  memset(signalState, 0, sizeof(signalState));
  sigfillset(&blockedSig);
}

/*
===================================================================================================
signalHandler
===================================================================================================
*/
static void 
signalHandler(int sig) {
	// not handler this signal, ignore
  if (sig < 0 || sig > MAX_SIGNAL || !signalState[sig].handler) {
    signal(sig, SIG_IGN);
    return;
  }
	// put signal in queue
  if (!signalState[sig].count && (signalQueueLen<MAX_SIGNAL)) {
    signalQueue[signalQueueLen++] = sig;
  }
	// add signal count
  signalState[sig].count++;
  signal(sig, signalHandler);
}

/*
===================================================================================================
SpeSignalRegister
===================================================================================================
*/
void 
SpeSignalRegister(int sig, SpeSignal_Handler handler) {
	if (sig < 0 || sig > MAX_SIGNAL) return;
	signalState[sig].count = 0;
 	if (handler == NULL) handler = SIG_IGN; 
	// set signal handler
	if (handler != SIG_IGN && handler != SIG_DFL) {
		signalState[sig].handler = handler;
 		signal(sig, signalHandler);
 	} else {                        
   	signalState[sig].handler = NULL;
 		signal(sig, handler);
	}
}

/*
===================================================================================================
SpeSignalProcess
===================================================================================================
*/
void 
SpeSignalProcess() {
  if (signalQueueLen == 0) return;
	sigset_t old_sig;
	sigprocmask(SIG_SETMASK, &blockedSig, &old_sig);
 	// check signal queue	
	for (int i=0; i<signalQueueLen; i++) {
		int sig = signalQueue[i];
 		speSignal_t* desc = &signalState[sig];
		if (desc->count) {
   		if (desc->handler) desc->handler(sig);
   		desc->count = 0;
 		}
 	}
	signalQueueLen = 0;
	sigprocmask(SIG_SETMASK, &old_sig, NULL);  
}

speModule_t speSignalModule = {
  "speSignal",
  0,
  SPE_CORE_MODULE,
  signalInit,
  NULL,
  NULL,
  NULL,
};
