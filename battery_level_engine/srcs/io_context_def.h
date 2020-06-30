#if !defined(_IOC_DEF_H_)
#define _IOC_DEF_H_

#include <stdint.h>

typedef void  ioc_handle_t;
typedef void  ioc_data_t;

typedef void (*ioc_event_func_t)(int, uint32_t, ioc_data_t*);
typedef void (*ioc_timeout_func_t)(ioc_data_t*);
typedef void (*ioc_data_free_func_t)(ioc_data_t*);

#endif // _IOC_DEF_H_