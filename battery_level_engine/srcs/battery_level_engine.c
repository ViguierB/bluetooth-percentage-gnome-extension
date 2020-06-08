#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define _BLE_INTERNAL_
#include "./battery_level_engine.h"

#ifndef NDEBUG
# include <stdio.h>
#endif

#define HANDFREEUNIT_HANDLE 0x10008

#define BLE_END_LINE "\r\n\r\nOK"

#define _ble_send(ble, hdv_data) __internal_ble_send(ble->sock, "\r\n" hdv_data "\r\n", sizeof(hdv_data) + 4, 0)

#ifndef NDEBUG

ssize_t __internal_ble_send(int __fd, const void *__buf, size_t __n, int __flags) {
  fprintf(stderr, "[TO DEVICE] %.*s\n", (int)__n, (char*)__buf);
  return send(__fd, __buf, __n, __flags);
}

#else

#define __internal_ble_send send

#endif


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


int ble_find_rfcomm_channel(ble_t *ble, const char* addr) {
  bdaddr_t target;
  sdp_list_t *attrid, *search, *seq, *next;
  sdp_session_t *session = 0;
  uint32_t range = 0x0000ffff;
  uuid_t group = { 0 };
  uint8_t port = 0;

  str2ba(addr, &target);
  sdp_uuid16_create(&group, HANDSFREE_SVCLASS_ID);

  session = sdp_connect(BDADDR_ANY, &target, SDP_RETRY_IF_BUSY);

  if (!session) {
    ble->ble_error_message = strerror(errno);
    return -1;
  }

  attrid = sdp_list_append(0, &range);
  search = sdp_list_append(0, &group);
  if (sdp_service_search_attr_req(session, search, SDP_ATTR_REQ_RANGE, attrid, &seq)) {
    ble->ble_error_message = strerror(errno);
    sdp_close(session);
    return -1;
  }
  sdp_list_free(attrid, 0);
  sdp_list_free(search, 0);

  for (; seq; seq = next) {
    sdp_record_t *rec = (sdp_record_t *) seq->data;
    sdp_list_t *proto_list = NULL;

    next = seq->next;
    // get a list of the protocol sequences
    if(sdp_get_access_protos(rec, &proto_list) == 0 ) {
      // get the RFCOMM port number
      port = sdp_get_proto_port(proto_list, RFCOMM_UUID);
      next = NULL; //force stop

      sdp_list_foreach(proto_list, (sdp_list_func_t)sdp_list_free, NULL);
      sdp_list_free(proto_list, NULL);
    }
    free(seq);
    sdp_record_free(rec);
  }

  sdp_close(session);

  if(port != 0) {
#   ifndef NDEBUG
      fprintf(stderr, "rfcomm channel: %d\n", port);
#   endif
    ble->channel = port;
    return 0;
  } else {
    ble->ble_error_message = "Device not compatible";
    return -1;
  }
}

int ble_connect_to(ble_t* ble, char* addr) {
  if (ble->channel == 0) {
    ble->ble_error_message = "No channel found, please run 'ble_find_rfcomm_channel()' before";
  }

  ble->sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  if (ble->sock < 0) {
    ble->ble_error_message = strerror(errno);
    return -1;
  }
  ble->rem_addr.rc_family = AF_BLUETOOTH;
  ble->rem_addr.rc_channel = (uint8_t) ble->channel;
  str2ba(addr, &ble->rem_addr.rc_bdaddr);

  int connect_res = connect(ble->sock, (struct sockaddr *)&ble->rem_addr, sizeof(ble->rem_addr));

  if (connect_res < 0) {
    ble->ble_error_message = strerror(errno);
    return connect_res;
  }
  ble->is_connected = 1;
  return 0;
}


int ble_hfp_nogiciate(ble_t* ble) {
  if (ble->is_connected) {
    ssize_t recv_len = 0;

    while ((recv_len = recv(ble->sock, ble->buffer, BUFFER_SIZE, 0)) >= 0) {
#     ifndef NDEBUG
        fprintf(stderr, "[FROM DEVICE] %.*s\n", (int)recv_len, ble->buffer);
#     endif
      if (_ble_strstr(ble, "BRSF", recv_len)) {
        _ble_send(ble, "+BRSF:20"BLE_END_LINE);
      } else if(_ble_strstr(ble, "CIND=", recv_len)) {
        _ble_send(ble, "+CIND: (\"battchg\",(0-5))"BLE_END_LINE);
      } else if (_ble_strstr(ble, "CIND?", recv_len)) {
        _ble_send(ble, "+CIND: 5"BLE_END_LINE);
      } else if (_ble_strstr(ble, "AT+XAPL", recv_len)) {
        _ble_send(ble, "+XAPL=iPhone,5"BLE_END_LINE); // lel
      } else if (_ble_strstr(ble, "AT+APLSIRI?", recv_len)) {
        _ble_send(ble, "+APLSIRI=0"BLE_END_LINE);
        return 0;
      } else {
        _ble_send(ble, "OK");
      }
    }

    ble->ble_error_message = "Device not compatible";
    return -1;
  } else {
    ble->ble_error_message = "Not connect to a device: call ble_connect_to() before";
    return -1;
  }
}

int ble_get_battery_level(ble_t* ble) {
  if (ble->ready) {
    ble->ble_error_message = "You must call 'ble_hfp_negociate()' before";
    return -1;
  }
  if (ble->is_connected) {
    ssize_t recv_len = 0;

    while ((recv_len = recv(ble->sock, ble->buffer, BUFFER_SIZE, 0)) >= 0) {
#     ifndef NDEBUG
        fprintf(stderr, "[FROM DEVICE] %.*s\n", (int)recv_len, ble->buffer);
#     endif
      if (_ble_strstr(ble, "IPHONEACCEV", recv_len)) {
        _ble_send(ble, "OK");
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
    return -1;
  } else {
    ble->ble_error_message = "Not connect to a device: call ble_connect_to() before";
    return -1;
  }
}

const char* ble_get_last_error_message(ble_t *ble) {
  return ble->ble_error_message;
}