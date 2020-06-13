#if !defined(_IOC_H_)
#define _IOC_H_

#include <stdint.h>

typedef void* ioc_handle_t;
typedef void* ioc_data_t;

typedef void (*ioc_event_func_t)(int, uint32_t, ioc_data_t);
typedef void (*ioc_timeout_func_t)(ioc_data_t);
typedef void (*ioc_data_free_func_t)(ioc_data_t);

#if defined(IOC_INTERNAL)

  enum ioc_type {
    IOC_TYPE_EVENT,
    IOC_TYPE_TIMEOUT
  };

  struct ioc_event_data_s {
    struct ioc_event_data_s*  prev;
    struct ioc_event_data_s*  next;
    enum ioc_type             type;
    union {
      ioc_event_func_t        fd_func;
      ioc_timeout_func_t      timeout_func;
    }                         func;
    union {
      int                     fd;
      int                     timeout;
    }                         value;
    ioc_data_free_func_t      free_data_func;
    ioc_data_t                data;
  };

#endif

typedef struct io_context_s {

# if defined(IOC_INTERNAL)
    int                       epoll_fd;
    struct epoll_event        events[IOC_MAX_EVENTS];
    char*                     last_error_str;
    struct ioc_event_data_s*  event_data_list;
    struct ioc_event_data_s*  timeout_data_list;
# endif

} io_context_t;

io_context_t* create_io_context();
void          delete_io_context(io_context_t* ioc);
int           ioc_wait_once(io_context_t* ioc);
ioc_handle_t  ioc_add_fd(io_context_t* ioc, int fd, uint32_t events, ioc_event_func_t handler, ioc_data_t data);
ioc_handle_t  ioc_get_handle(ioc_data_t* data);
int           ioc_remove_handle(io_context_t* ioc, ioc_handle_t handle);
const char*   ioc_get_last_error(io_context_t* ioc);
ioc_handle_t  ioc_timeout(io_context_t* ioc, int timeout, ioc_timeout_func_t handler, ioc_data_t data);
void          ioc_set_handle_delete_func(ioc_handle_t handle, ioc_data_free_func_t free_func);

#endif // _IOC_H_
