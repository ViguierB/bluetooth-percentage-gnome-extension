#if !defined(_IOC_H_)
#define _IOC_H_

#include "./io_context_def.h"

#if !defined(IOC_INTERNAL)

typedef void  io_context_t;

#endif

io_context_t* create_io_context();
void          delete_io_context(io_context_t* ioc);
int           ioc_wait_once(io_context_t* ioc);
ioc_handle_t  ioc_add_fd(io_context_t* ioc, int fd, uint32_t events, ioc_event_func_t handler, ioc_data_t data);
ioc_handle_t  ioc_get_handle(ioc_data_t* data);
io_context_t* ioc_get_context_from_handle(ioc_handle_t handle);
int           ioc_remove_handle(ioc_handle_t handle);
const char*   ioc_get_last_error(io_context_t* ioc);
ioc_handle_t  ioc_timeout(io_context_t* ioc, int timeout, ioc_timeout_func_t handler, ioc_data_t data);
void          ioc_set_handle_delete_func(ioc_handle_t handle, ioc_data_free_func_t free_func);

#endif // _IOC_H_
