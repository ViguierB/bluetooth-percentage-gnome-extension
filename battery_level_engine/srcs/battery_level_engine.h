#if !defined(_BLE_H_)
#define _BLE_H_

enum ble_events_e {
  BLE_ERROR,
  BLE_BATTERY_LEVEL,
  __BLE_EVENTS_ENUM_LEN__
};

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
  ioc_handle_t    ioc_sock_handle;
  sockaddr_rc_t   rem_addr;
  uint8_t         channel;
  void*           event_handlers[__BLE_EVENTS_ENUM_LEN__];
  char            is_connected;
  char            ready;
  char*           ble_error_message;
  char            buffer[BUFFER_SIZE + 1]; // need one extra char in some cases
} ble_t;
# define BLE ble_t

char*         _ble_strstr(const ble_t* ble, const char* find, size_t slen);
char**        split(char *inp, char sep, int *nb_lines);
void          free_split(char** sstr);

#else
# include "./io_context.h"
# define BLE void
#endif

typedef void  (*ble_on_level_change_handler_t)(BLE*, int level);
typedef void  (*ble_on_error_handler_t)(BLE*, char *error);

BLE*        create_battery_level_engine(io_context_t*);
int         delete_battery_level_engine(BLE*);
int         ble_find_rfcomm_channel(BLE*, const char*);
void        ble_register_event_handler(BLE* ble, enum ble_events_e e, void* event_handler);
int         ble_connect_to(BLE*, char* addr);
int         ble_get_battery_level(BLE*);
const char* ble_get_last_error_message(BLE*);
int         ble_hfp_nogiciate(BLE*);
int         ble_send_command(BLE*, const char* cmd);

#endif // _BLE_H_
