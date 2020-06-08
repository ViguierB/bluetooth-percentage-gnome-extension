#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#define IOC_MAX_EVENTS 10
#define IOC_INTERNAL
#include "./io_context.h"

#define OFFSET_OF(ptype, memb) ((uintptr_t)&(((ptype*)0)->memb))

io_context_t* create_io_context() {
  io_context_t* ioc = calloc(1, sizeof(io_context_t));
  
  if (!ioc) { return NULL; }
  ioc->epoll_fd = epoll_create1(0);
  if (ioc->epoll_fd == -1) { return NULL; }
  return ioc;
}

void  delete_io_context(io_context_t* ioc) {
  close(ioc->epoll_fd);
  free(ioc);
}

int  ioc_wait_once(io_context_t* ioc) {
  int                 nfds;

  nfds = epoll_wait(ioc->epoll_fd, ioc->events, IOC_MAX_EVENTS, -1);
  if (nfds == -1) {
    ioc->last_error_str = strerror(errno);
    return -1;
  }

  for (int i = 0; i < nfds; ++i) {
      struct ioc_event_data_s* d_event = ioc->events[i].data.ptr;
      
      d_event->func(ioc->events[i].events, &d_event->data_wrap);
  }
  return 0;
}

ioc_handle_t ioc_add_fd(io_context_t* ioc, int fd, uint32_t events, ioc_event_func_t handler, ioc_data_t data) {
  struct epoll_event        ev = { 0 };
  struct ioc_event_data_s*  d_event = malloc(sizeof (struct ioc_event_data_s));

  if (!d_event) {
    ioc->last_error_str = "memory out";
    return NULL;
  }

  // d_event->prev = NULL;
  // d_event->next = NULL;
  d_event->func = handler;
  d_event->fd = fd; // end user -> read only
  d_event->free_data_func = NULL;
  d_event->data_wrap.data = data;
  d_event->data_wrap.fd = fd;  // end user -> ! read only

  ev.events = events || EPOLLIN;
  ev.data.ptr = d_event;
  if (epoll_ctl(ioc->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    ioc->last_error_str = strerror(errno);
    free(d_event);
    return NULL;
  }
  return (void*)d_event;
}

ioc_handle_t ioc_get_handle(ioc_data_wrap_t* data) {
  const uintptr_t wrap_offset = OFFSET_OF(struct ioc_event_data_s, data_wrap);

  return (void*)((uintptr_t)data - wrap_offset);
}

int ioc_remove_fd(io_context_t* ioc, ioc_handle_t handle) {
  struct ioc_event_data_s*  d_event = handle;

  if (epoll_ctl(ioc->epoll_fd, EPOLL_CTL_DEL, d_event->fd, NULL) < 0) {
    ioc->last_error_str = strerror(errno);
    return -1;
  }
  if (!!d_event->free_data_func) {
    d_event->free_data_func(d_event->data_wrap.data);
  }
  free(d_event);
  return 0;
}

void  ioc_set_handle_delete_func(ioc_handle_t handle, ioc_data_free_func_t free_func) {
  struct ioc_event_data_s*  d_event = handle;

  d_event->free_data_func = free_func;
}

// int io_add(io_context_t* ioc, ioc_event_func_t* handler, ioc_data_t data) {

// }

const char* ioc_get_last_error(io_context_t* ioc) {
  return ioc->last_error_str;
}

