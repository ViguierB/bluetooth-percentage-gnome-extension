#if !defined(_SIGNALS_H_)
#define _SIGNALS_H_

#include <signal.h>
#include "signals_def.h"

#define DO_PRAGMA(x) _Pragma (#x)

#if !defined(SIGNALS_INTERNAL)

typedef void  signal_set_t;
typedef void  signals_t;

#endif

#define __signal_create_set(in_set) \
  ({ \
    DO_PRAGMA (GCC diagnostic push) \
    DO_PRAGMA (GCC diagnostic ignored "-Wincompatible-pointer-types") \
    void* res = signal_create_set_(sizeof(in_set) / sizeof(signal_set_elem_t), (in_set)); \
    DO_PRAGMA (GCC diagnostic pop) \
    res; \
  })
#define signal_create_set(...) __signal_create_set(((signal_set_elem_t[]){ __VA_ARGS__ }))

#define __signal_set_append(source, in_set) \
  ({ \
    DO_PRAGMA (GCC diagnostic push) \
    DO_PRAGMA (GCC diagnostic ignored "-Wincompatible-pointer-types") \
    void* res = signal_set_append_(source, sizeof(in_set) / sizeof(signal_set_elem_t), (in_set)); \
    DO_PRAGMA (GCC diagnostic pop) \
    res; \
  })
#define signal_set_append(source, ...) __signal_set_append((source), ((signal_set_elem_t[]){ __VA_ARGS__ }))

signals_t*    signal_create(io_context_t* ctx);
void          signal_delete(signals_t* s);
signal_set_t* signal_create_set_(uint32_t len, signal_set_elem_t in_set[]);
signal_set_t* signal_create_set_empty(void);
signal_set_t* signal_set_append_(signal_set_t *source, uint32_t len, signal_set_elem_t in_set[]);
int           signal_set(signals_t* s, signal_set_t* set);

#endif // _SIGNALS_H_
