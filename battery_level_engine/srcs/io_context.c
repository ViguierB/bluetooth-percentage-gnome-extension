#include <stdlib.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define IOC_INTERNAL

typedef void* (*io_context_event_handler_t)(void*);
typedef struct io_context_s {

# if defined(IOC_INTERNAL)
    
# endif

} io_context_t;

io_context_t* io_init() {
  return calloc(1, sizeof(io_context_t));
}

io_run(io_context_t* ioc) {
  int epoll_fd = epoll_create1(0);

  if (epoll_fd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }
}

io_add_fd(io_context_t* ioc, int fd, io_context_event_handler_t* handler, void* data) {

}

io_add(io_context_t* ioc, io_context_event_handler_t* handler, void* data) {

}

