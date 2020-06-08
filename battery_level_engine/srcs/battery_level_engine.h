#if !defined(_BLE_)
#define _BLE_

#if defined(_BLE_INTERNAL_)
# define BUFFER_SIZE 128

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

typedef int socket_t;
typedef struct sockaddr_rc sockaddr_rc_t;

typedef struct ble_s {
  socket_t        sock;
  sockaddr_rc_t   rem_addr;
  uint8_t         channel;
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
# define BLE void
#endif

BLE*        create_battery_level_engine();
int         delete_battery_level_engine(BLE*);
int         ble_find_rfcomm_channel(BLE*, const char*);
int         ble_connect_to(BLE*, char* addr);
int         ble_get_battery_level(BLE*);
const char* ble_get_last_error_message(BLE*);
int         ble_hfp_nogiciate(BLE*);

#endif // _BLE_
