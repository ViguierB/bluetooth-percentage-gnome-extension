#if !defined(__BATTERY_LEVEL_ENGINE_H__)
#define __BATTERY_LEVEL_ENGINE_H__

#include "io_context.h"

struct ctx_data {
  void*         cm;
  io_context_t* ctx;
  int           stop;
  char          buffer[1024];
};

void  on_stdin_data_pending(int fd, uint32_t events, struct ctx_data* data);
char* str_trim_and_dup(const char* source);

#endif // __BATTERY_LEVEL_ENGINE_H__
