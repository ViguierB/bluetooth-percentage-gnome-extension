#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <errno.h>

#define BUFFER_SIZE 128

#define _ble_send(ble, hdv_data) send(ble->sock, "\r\n" hdv_data "\r\n", sizeof(hdv_data) + 4, 0)

typedef int socket_t;
typedef struct sockaddr_rc sockaddr_rc_t;

char  **split(char *inp, char sep, int *nb_lines);
void  free_split(char** sstr);

typedef struct ble_s {
  socket_t        sock;
  sockaddr_rc_t   rem_addr;
  char            is_connected;
  char            used;
  char*           ble_error_message;
  char            buffer[BUFFER_SIZE + 1]; // need one extra char in some cases
} ble_t;

static char* _ble_strstr(const ble_t* ble, const char* find, size_t slen)
{
  char        c, sc;
  const char* s = ble->buffer;
  size_t      len;

  if ((c = *find++) != '\0') {
    len = strlen(find);
    do {
      do {
        if (slen-- < 1 || (sc = *s++) == '\0')
          return (NULL);
      } while (sc != c);
      if (len > slen)
        return (NULL);
    } while (strncmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}


ble_t* create_battery_level_engine() {
  ble_t* ble = calloc(1, sizeof(ble_t));

  if (!ble) {
    ble->ble_error_message = strerror(errno);
  }
  return ble;
}

int   delete_battery_level_engine(ble_t* ble) {
  if (ble->is_connected) {
    if (close(ble->sock) < 0) {
      ble->ble_error_message = strerror(errno);
      return -1;
    }
  }

  free(ble);
  return 0;
}

int   ble_connect_to(ble_t* ble, char* addr, uint16_t channel) {
  ble->sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  if (ble->sock < 0) { return -1; }

  ble->rem_addr.rc_family = AF_BLUETOOTH;
  ble->rem_addr.rc_channel = (uint8_t) channel;
  str2ba(addr, &ble->rem_addr.rc_bdaddr);

  int connect_res = connect(ble->sock, (struct sockaddr *)&ble->rem_addr, sizeof(ble->rem_addr));

  if (connect_res < 0) {
    ble->ble_error_message = strerror(errno);
    return connect_res;
  }
  ble->is_connected = 1;
  return 0;
}

int   ble_get_battery_level(ble_t* ble) {
  if (ble->used) {
    ble->ble_error_message = "This object as been used, recreate a new one for making a new request";
    return -1;
  }
  if (ble->is_connected) {
    ssize_t recv_len = 0;

    while ((recv_len = recv(ble->sock, ble->buffer, BUFFER_SIZE, 0)) >= 0) {
      if (_ble_strstr(ble, "BRSF", recv_len)) {
        _ble_send(ble, "+BRSF:20");
        _ble_send(ble, "OK");
      } else if(_ble_strstr(ble, "CIND=", recv_len)) {
        _ble_send(ble, "+CIND: (\"battchg\",(0-5))");
        _ble_send(ble, "OK");
      } else if (_ble_strstr(ble, "CIND?", recv_len)) {
        _ble_send(ble, "+CIND: 5");
        _ble_send(ble, "OK");
      } else if (_ble_strstr(ble, "IPHONEACCEV", recv_len)) {
        if (!_ble_strstr(ble, ",", recv_len)) { continue; }
        ble->buffer[recv_len] = '\0';
        /* https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf */
        
        int n;
        char* iphoneaccev_values = strchr(ble->buffer, '=');

        if (iphoneaccev_values++) {
          char** commands = split(iphoneaccev_values, ',', &n);
          for (int i = 1; i < n - 1; i += 2) {
            char key = *(commands[i]);

            // 1 = Battery Level
            if (key == '1') {
              // Battery Level: string value between '0' and '9'
              int   value = atoi(commands[i + 1]);

              free_split(commands);
              ble->used = 1;
              return (value + 1) * 10;
            }
          }
          free_split(commands);
        }
      } else {
        _ble_send(ble, "OK");
      }
    }

    ble->ble_error_message = "Device not compatible";
    ble->used = 1;
    return -1;
  } else {
    ble->ble_error_message = "Not connect to a device: call ble_connect_to() before";
    return -1;
  }
}

const char* ble_get_last_error_message(ble_t *ble) {
  return ble->ble_error_message;
}