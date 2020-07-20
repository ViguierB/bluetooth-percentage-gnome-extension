#if !defined(_BLE_H_)
#define _BLE_H_

enum ble_events_e {
  BLE_ERROR,
  BLE_RFCOMM_CHANNEL_FOUND,
  BLE_READY,
  BLE_BATTERY_LEVEL,
  __BLE_EVENTS_ENUM_LEN__
};

#include <stdbool.h>

#if defined(_BLE_INTERNAL_)
# define BUFFER_SIZE 128

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include "./io_context.h"

typedef int socket_t;
typedef struct sockaddr_rc sockaddr_rc_t;

typedef struct ble_s {
  io_context_t*   ctx;
  socket_t        sock;
  ioc_handle_t*   ioc_sock_handle;
  sockaddr_rc_t   rem_addr;
  uint8_t         channel;
  struct {
    void*         handler;
    void*         data;
  }               event_handlers[__BLE_EVENTS_ENUM_LEN__];
  bool            is_connected;
  bool            ready;
  char*           ble_error_message;
  char            buffer[BUFFER_SIZE + 1]; // need one extra char in some cases
} ble_t;

char*         _ble_strstr(const ble_t* ble, const char* find, size_t slen);
char**        split(char *inp, char sep, int *nb_lines);
void          free_split(char** sstr);

#else
# include "./io_context.h"

typedef void ble_t;
#endif

typedef void  (*ble_on_level_change_handler_t)(ble_t*, int level, void* data);
typedef void  (*ble_on_ready_handler_t)(ble_t*, void* data);
typedef void  (*ble_on_error_handler_t)(ble_t*, char *error, void* data);
typedef void  (*ble_on_rfcomm_channel_find_ended_handler_t)(ble_t*, bool success, const char* addr, uint8_t channel, void* data);

ble_t*      create_battery_level_engine(io_context_t*);
int         delete_battery_level_engine(ble_t*);
int         ble_find_rfcomm_channel(ble_t*, const char*);
void        ble_register_event_handler(ble_t* ble, enum ble_events_e e, void* event_handler, void* data);
int         ble_connect_to(ble_t*, const char* addr, uint8_t channel);
int         ble_get_battery_level(ble_t*);
const char* ble_get_last_error_message(ble_t*);
int         ble_send_command(ble_t*, const char* cmd);

#endif // _BLE_H_
