#if !defined(_SIGNALS_DEF_H_)
#define _SIGNALS_DEF_H_

#include <sys/signalfd.h>
#include <stdint.h>

#include "io_context.h"

typedef struct {
  uint32_t  sig;
  void      (*hdl)(void* data, struct signalfd_siginfo* si);
  void*     data;
} signal_set_elem_t;

#endif // _SIGNALS_DEF_H_
