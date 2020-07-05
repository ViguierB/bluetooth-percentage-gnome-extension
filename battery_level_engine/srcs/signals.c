#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "signals_def.h"

typedef struct {
  uint32_t            size;
  sigset_t            old;
  signal_set_elem_t   elems[];
} signal_set_t;

typedef struct signals_s {
  int                     signal_fd;
  struct signalfd_siginfo fdsi;
  io_context_t*           ctx;
  signal_set_t*           set;
  ioc_handle_t*           handle;
} signals_t;

#define SIGNALS_INTERNAL
#include "signals.h"

signals_t*  signal_create(io_context_t* ctx) {
  signals_t* sig = malloc(sizeof(signals_t));

  if (sig == NULL) { return NULL; }
  sig->ctx = ctx;
  sig->set = NULL;
  sig->signal_fd = -1;
  return sig;
}

void  signal_delete(signals_t* s) {
  if (!!s->set) {
    ioc_remove_handle(s->handle);
    free(s->set);
  }

  if (s->signal_fd != -1) {
    close(s->signal_fd);
  }

  free(s);
}

signal_set_t* signal_create_set_(uint32_t len, signal_set_elem_t in_set[len]) {
  signal_set_t* set = malloc(sizeof(signal_set_t) + (sizeof(signal_set_elem_t) * len));

  if (set == NULL) { return NULL; }
  set->size = len;
  memcpy(set->elems, in_set, sizeof(signal_set_elem_t) * len);
  return set;
}

signal_set_t* signal_create_set_empty(void) {
  return signal_create_set_(0, (signal_set_elem_t[]) {});
}

signal_set_t* signal_set_append_(signal_set_t *source, uint32_t len, signal_set_elem_t in_set[len]) {
  size_t        new_size = source->size + len;
  size_t        last_size = source->size;
  signal_set_t* set = realloc(source, sizeof(signal_set_t) + (sizeof(signal_set_elem_t) * new_size));

  if (set == NULL) { return NULL; }
  set->size = new_size;
  memcpy(&(set->elems[last_size]), in_set, sizeof(signal_set_elem_t) * len);
  return set;
}

static void _internal_on_sig_recvd(int fd, uint32_t event, signals_t* s) {
  (void) event;
  ssize_t ss = read(fd, &s->fdsi, sizeof(struct signalfd_siginfo));

  assert(ss == sizeof(struct signalfd_siginfo));
  (void)ss;
  
  for (uint32_t i = 0; i < s->set->size; ++i) {
    if (s->set->elems[i].sig == s->fdsi.ssi_signo) {
      s->set->elems[i].hdl(s->set->elems[i].data, &s->fdsi);
      return;
    }
  }

  assert(0); // unexpected signal catched !! 
}

int signal_set(signals_t* s, signal_set_t* set) {
  sigset_t  mask;

  if (!!s->set) {
    if (sigprocmask(SIG_SETMASK, &s->set->old, NULL) == -1) {
      return -1;
    }
    ioc_remove_handle(s->handle);
    free(s->set);
  }

  s->set = set;
  sigemptyset(&mask);
  for (uint32_t i = 0; i < set->size; ++i) {
    sigaddset(&mask, set->elems[i].sig);
  }

  if (sigprocmask(SIG_BLOCK, &mask, &s->set->old) == -1)
    return -1;

  s->signal_fd = signalfd(s->signal_fd, &mask, 0);
  if (s->signal_fd == -1) {
    return -1;
  }

  s->handle = ioc_add_fd(s->ctx, s->signal_fd, EPOLLIN, (ioc_event_func_t)&_internal_on_sig_recvd, s);
  return 0;
}

