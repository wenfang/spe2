#ifndef __SPE_HANDLER_H
#define __SPE_HANDLER_H

typedef void (*SpeHandler0)(void);
typedef void (*SpeHandler1)(void*);
typedef void (*SpeHandler2)(void*, void*);

typedef struct spe_handler_s {
  void* handler;
  void* arg1;
  void* arg2;
  int   argc;
} __attribute__((aligned(sizeof(long)))) spe_handler_t;

#define SPE_HANDLER_NULL                        (spe_handler_t){NULL, NULL, NULL, 0}
#define SPE_HANDLER0(handler)                   (spe_handler_t){handler, NULL, NULL, 0}
#define SPE_HANDLER1(handler, arg1)             (spe_handler_t){handler, arg1, NULL, 1}
#define SPE_HANDLER2(handler, arg1, arg2)       (spe_handler_t){handler, arg1, arg2, 2}

#define SPE_HANDLER_CALL(h)                                         \
  do {                                                              \
    if (!(h).handler) break;                                        \
    switch ((h).argc) {                                             \
    case 0:                                                         \
      ((SpeHandler0)((h).handler))();                               \
      break;                                                        \
    case 1:                                                         \
      ((SpeHandler1)((h).handler))((h).arg1);                       \
      break;                                                        \
    case 2:                                                         \
      ((SpeHandler2)((h).handler))((h).arg1, (h).arg2);             \
      break;                                                        \
    }                                                               \
  } while(0)

#endif
