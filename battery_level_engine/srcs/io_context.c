#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/epoll.h>

#include "./circular_double_linked_list.h"
#define IOC_MAX_EVENTS 10
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

int  ioc_wait_once(io_context_t* ioc) {
  int nfds;
  int timeout = -1;

  if (!!ioc->timeout_data_list) {
    timeout = ioc->timeout_data_list->value.fd;
  }

  nfds = epoll_wait(ioc->epoll_fd, ioc->events, IOC_MAX_EVENTS, timeout);
  if (nfds == -1) {
    ioc->last_error_str = strerror(errno);
    return -1;
  } else if (nfds == 0) {
    clock_t                   start = clock();
    struct ioc_event_data_s*  d_event = ioc->timeout_data_list;
    int                       d_ev_timeout = d_event->value.timeout;

    ioc->timeout_data_list = list_ioc_event_data_t_remove(d_event);
    d_event->func.timeout_func(d_event->data);
    if (!!d_event->free_data_func) {
      d_event->free_data_func(d_event->data);
    }
    free(d_event);

    if (ioc->timeout_data_list) {

      struct ioc_event_data_s* first, *cur;

      cur = first = ioc->timeout_data_list;

      long elapsed;
      elapsed = ((double)clock() - start) / CLOCKS_PER_SEC * 1000;
      do {
        cur->value.timeout -= d_ev_timeout + elapsed;
        cur = cur->next;
      } while (cur != first);
    }

    return 0;
  }

  for (int i = 0; i < nfds; ++i) {
      struct ioc_event_data_s* d_event = ioc->events[i].data.ptr;
      
      d_event->func.fd_func(d_event->value.fd, ioc->events[i].events, d_event->data);
  }
  return 0;
}

ioc_handle_t ioc_add_fd(io_context_t* ioc, int fd, uint32_t events, ioc_event_func_t handler, ioc_data_t data) {
  struct epoll_event        ev = { 0 };
  struct ioc_event_data_s*  d_event = malloc(sizeof (struct ioc_event_data_s));

  if (!d_event) {
    ioc->last_error_str = "out of memory";
    return NULL;
  }

  d_event->prev = d_event; //important (requested by the linked list)
  d_event->next = d_event; //important (requested by the linked list)
  d_event->type = IOC_TYPE_EVENT;
  d_event->func.fd_func = handler;
  d_event->value.fd = fd; // end user -> hidden
  d_event->free_data_func = NULL;
  d_event->data = data;

  if (ioc->event_data_list) {
    list_ioc_event_data_t_add_to(ioc->event_data_list, d_event);
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

ioc_handle_t  ioc_timeout(io_context_t* ioc, int timeout, ioc_timeout_func_t handler, ioc_data_t data) {
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
  d_event->func.timeout_func = handler;
  d_event->value.timeout = timeout;
  d_event->data = data;
  d_event->free_data_func = NULL;

  if (ioc->timeout_data_list == NULL) {
    ioc->timeout_data_list = d_event;
  } else {
    struct ioc_event_data_s*  cur = NULL;
    struct ioc_event_data_s*  first = NULL;

    first = cur = ioc->timeout_data_list;
    if (first->value.timeout >= timeout) {
      list_ioc_event_data_t_add_to(first->prev, d_event);
      ioc->timeout_data_list = d_event;
    } else if (timeout >= first->prev->value.timeout) {
      list_ioc_event_data_t_add_to(first->prev, d_event);
    } else {
      do {
        if (cur->next->value.timeout >= timeout) {
          list_ioc_event_data_t_add_to(cur, d_event);
          break;
        }
        cur = cur->next;
      } while (cur != first);
    }
  }
  return (void*)d_event;
}

ioc_handle_t ioc_get_handle(ioc_data_t* data) {
  const uintptr_t data_offset = OFFSET_OF(struct ioc_event_data_s, data);

  return (void*)((uintptr_t)data - data_offset);
}

int ioc_remove_handle(io_context_t* ioc, ioc_handle_t handle) {
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
  } else {
    ioc->timeout_data_list = list_ioc_event_data_t_remove(d_event);
  }
  free(d_event);
  return 0;
}

void  ioc_set_handle_delete_func(ioc_handle_t handle, ioc_data_free_func_t free_func) {
  struct ioc_event_data_s*  d_event = handle;

  d_event->free_data_func = free_func;
}

const char* ioc_get_last_error(io_context_t* ioc) {
  return ioc->last_error_str;
}

