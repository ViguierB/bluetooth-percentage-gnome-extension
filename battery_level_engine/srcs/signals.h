#if !defined(_SIGNALS_H_)
#define _SIGNALS_H_

#include <signal.h>
#include "signals_def.h"

#define MAKE_SIGNAL_SET_ELEM(_sig, _hdl, _data) { (uint32_t)_sig, (void(*)(void*, struct signalfd_siginfo*))((void*)_hdl), (void*)_data }

#if !defined(SIGNALS_INTERNAL)

typedef void  signal_set_t;
typedef void  signals_t;

#endif

#define __signal_create_set(in_set) signal_create_set_(sizeof(in_set) / sizeof(signal_set_elem_t), (in_set))
#define signal_create_set(...) __signal_create_set(((signal_set_elem_t[]){ __VA_ARGS__ }))

signals_t*    signal_create(io_context_t* ctx);
void          signal_delete(signals_t* s);
signal_set_t* signal_create_set_(uint32_t len, signal_set_elem_t in_set[]);
signal_set_t* signal_create_set_empty(void);
signal_set_t* signal_set_append(signal_set_t *source, signal_set_elem_t in_set);
int           signal_set(signals_t* s, signal_set_t* set);

#endif // _SIGNALS_H_
