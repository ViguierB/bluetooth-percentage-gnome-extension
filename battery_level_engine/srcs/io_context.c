#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/epoll.h>
#include <stdbool.h>

#include "./circular_double_linked_list.h"
#include "./io_context_def.h"

#define IOC_MAX_EVENTS 10

enum ioc_type {
  IOC_TYPE_EVENT,
  IOC_TYPE_TIMEOUT
};

struct ioc_event_data_s {
  struct ioc_event_data_s*  prev;
  struct ioc_event_data_s*  next;
  enum ioc_type             type;
  void*                     context;
  union {
    ioc_event_func_t          fd_func;
    ioc_timeout_func_t        timeout_func;
  }                         func;
  union {
    int                       fd;
    struct {
      int                       val;
      bool                      take_care_later;
    }                         timeout;
  }                         value;
  ioc_data_free_func_t      free_data_func;
  ioc_data_t*               data;
};

typedef struct io_context_s {

  int                       epoll_fd;
  struct epoll_event        events[IOC_MAX_EVENTS];
  char*                     last_error_str;
  struct ioc_event_data_s*  event_data_list;
  struct ioc_event_data_s*  timeout_data_list;
  bool                      is_runnning;

} io_context_t;

#define IOC_INTERNAL
#include "./io_context.h"

typedef struct ioc_event_data_s ioc_event_data_t;
DECLARE_CIRCULAR_DOUBLE_LINKED_LIST(ioc_event_data_t)

#define OFFSET_OF(ptype, memb) ((uintptr_t)&(((ptype*)0)->memb))

io_context_t* create_io_context() {
  io_context_t* ioc = calloc(1, sizeof(io_context_t));
  
  if (!ioc) { return NULL; }
  ioc->epoll_fd = epoll_create1(0);
  if (ioc->epoll_fd == -1) { return NULL; }
  return ioc;
}

static inline void  __clean_list(ioc_event_data_t* list) {
  ioc_event_data_t* first, *cur;

  first = cur = list;
  if (!!first) {
    do {
      void* free_later = cur;
      if (!!cur->free_data_func) {
        cur->free_data_func(cur->data);
      }
      cur = cur->next;
      free(free_later);
    } while (cur != first);
  }
}

void  delete_io_context(io_context_t* ioc) {
  __clean_list(ioc->event_data_list);
  __clean_list(ioc->timeout_data_list);
  close(ioc->epoll_fd);
  free(ioc);
}

static inline uint64_t  __timespec_to_ms(struct timespec *a) {
  return ((((uint64_t) a->tv_sec * 10000) + (a->tv_nsec / 100000)) / 10);
}

static inline uint64_t  __now(void) {
  struct timespec now;

  clock_gettime(
#   if defined(CLOCK_BOOTTIME)
      CLOCK_BOOTTIME
#   elif defined (CLOCK_MONOTNIC)
      CLOCK_MONOTONIC
#   else
#     error "Your os is not compatible with posix time"
      0
#   endif
    , &now);

  return (__timespec_to_ms(&now));
}



int  ioc_wait_once(io_context_t* ioc) {
  int       nfds;
  int       timeout = -1;
  uint64_t  start = __now();

  ioc->is_runnning = true;

  if (!!ioc->timeout_data_list) {
    timeout = ioc->timeout_data_list->value.timeout.val;
  }

  nfds = epoll_wait(ioc->epoll_fd, ioc->events, IOC_MAX_EVENTS, timeout);
  if (nfds == -1) {
    ioc->last_error_str = strerror(errno);
    ioc->is_runnning = false;
    return -1;
  } else if (nfds == 0) {
    struct ioc_event_data_s*  d_event = ioc->timeout_data_list;

    ioc->timeout_data_list = list_ioc_event_data_t_remove(d_event);
    d_event->func.timeout_func(d_event->data);
    if (!!d_event->free_data_func) {
      d_event->free_data_func(d_event->data);
    }
    free(d_event);

  } else {
    for (int i = 0; i < nfds; ++i) {
      struct ioc_event_data_s* d_event = ioc->events[i].data.ptr;
      
      d_event->func.fd_func(d_event->value.fd, ioc->events[i].events, d_event->data);
    }
  }

  if (ioc->timeout_data_list) {
    struct ioc_event_data_s* first, *cur;

    cur = first = ioc->timeout_data_list;

    long elapsed = __now() - start;
    do {
      if (cur->value.timeout.take_care_later != true) {
        cur->value.timeout.val -= elapsed;
        if (cur->value.timeout.val < 0) {
          cur->value.timeout.val = 0;
        }
      } else {
        cur->value.timeout.take_care_later = false;
      }
      cur = cur->next;
    } while (cur != first);
  }

  ioc->is_runnning = false;
  return 0;
}

io_context_t* ioc_get_context_from_handle(ioc_handle_t* handle) {
  return ((ioc_event_data_t*)handle)->context;
}

ioc_handle_t* ioc_add_fd(io_context_t* ioc, int fd, uint32_t events, ioc_event_func_t handler, ioc_data_t* data) {
  struct epoll_event        ev = { 0 };
  struct ioc_event_data_s*  d_event = malloc(sizeof (struct ioc_event_data_s));

  if (!d_event) {
    ioc->last_error_str = "out of memory";
    return NULL;
  }

  d_event->prev = d_event; //important (requested by the linked list)
  d_event->next = d_event; //important (requested by the linked list)
  d_event->type = IOC_TYPE_EVENT;
  d_event->context = ioc;
  d_event->func.fd_func = handler;
  d_event->value.fd = fd; // end user -> hidden
  d_event->free_data_func = NULL;
  d_event->data = data;

  if (ioc->event_data_list) {
    list_ioc_event_data_t_add_after(ioc->event_data_list, d_event);
  } else {
    ioc->event_data_list = d_event;
  }

  ev.events = events || EPOLLIN;
  ev.data.ptr = d_event;
  if (epoll_ctl(ioc->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    ioc->last_error_str = strerror(errno);
    free(d_event);
    return NULL;
  }
  return (void*)d_event;
}

ioc_handle_t* ioc_timeout(io_context_t* ioc, int timeout, ioc_timeout_func_t handler, ioc_data_t* data) {
  if (timeout < 0) {
    ioc->last_error_str = "timeout argument must be > 0";
    return NULL;
  }

  struct ioc_event_data_s*  d_event = malloc(sizeof (struct ioc_event_data_s));

  if (!d_event) {
    ioc->last_error_str = "out of memory";
    return NULL;
  }

  d_event->next = d_event;
  d_event->prev = d_event;
  d_event->type = IOC_TYPE_TIMEOUT;
  d_event->context = ioc;
  d_event->func.timeout_func = handler;
  d_event->value.timeout.val = timeout;
  d_event->value.timeout.take_care_later = ioc->is_runnning;
  d_event->data = data;
  d_event->free_data_func = NULL;

  if (ioc->timeout_data_list == NULL) {
    ioc->timeout_data_list = d_event;
  } else {
    struct ioc_event_data_s*  cur = NULL;
    struct ioc_event_data_s*  first = NULL;

    first = cur = ioc->timeout_data_list;
    if (first->value.timeout.val >= timeout) {
      list_ioc_event_data_t_add_before(first, d_event);
      ioc->timeout_data_list = d_event;
    } else if (timeout >= first->prev->value.timeout.val) {
      list_ioc_event_data_t_add_before(first, d_event);
    } else {
      do {
        if (cur->next->value.timeout.val >= timeout) {
          list_ioc_event_data_t_add_after(cur, d_event);
          break;
        }
        cur = cur->next;
      } while (cur != first);
    }
  }
  return (void*)d_event;
}

ioc_handle_t* ioc_get_handle(ioc_data_t* data) {
  const uintptr_t data_offset = OFFSET_OF(struct ioc_event_data_s, data);

  return (void*)((uintptr_t)data - data_offset);
}

int ioc_remove_handle(ioc_handle_t* handle) {
  io_context_t*             ioc = ioc_get_context_from_handle(handle);
  struct ioc_event_data_s*  d_event = handle;

  if (d_event->type == IOC_TYPE_EVENT && epoll_ctl(ioc->epoll_fd, EPOLL_CTL_DEL, d_event->value.fd, NULL) < 0) {
    ioc->last_error_str = strerror(errno);
    return -1;
  }
  if (!!d_event->free_data_func) {
    d_event->free_data_func(d_event->data);
  }
  if (d_event->type == IOC_TYPE_EVENT) {
    ioc->event_data_list = list_ioc_event_data_t_remove(d_event);
  } else if (d_event->type = IOC_TYPE_TIMEOUT) {
    if (ioc->timeout_data_list == d_event) {
      ioc->timeout_data_list = list_ioc_event_data_t_remove(d_event);
    } else {
      list_ioc_event_data_t_remove(d_event);
    }
  }
  free(d_event);
  return 0;
}

void  ioc_set_handle_delete_func(ioc_handle_t* handle, ioc_data_free_func_t free_func) {
  struct ioc_event_data_s*  d_event = handle;

  d_event->free_data_func = free_func;
}

const char* ioc_get_last_error(io_context_t* ioc) {
  return ioc->last_error_str;
}

