#if !defined(__CONNECTION_MANGER_H__)
#define __CONNECTION_MANGER_H__

#include "io_context.h"

typedef void  connection_t;
typedef void  cm_t;

cm_t*         cm_create(io_context_t* ctx);
void          cm_delete(cm_t* cm);
connection_t* cm_add_connection(cm_t* this, const char* addr);
cm_t*         cm_connection_get_manager(connection_t* connection);
const char*   cm_connection_get_addr(connection_t* connection);
void          cm_foreach_connections(cm_t* this, void (*h)(connection_t*, void*), void* data);
void          cm_connection_connect(connection_t* connection);

#endif // __CONNECTION_MANGER_H__