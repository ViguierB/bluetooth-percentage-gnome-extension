#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <stdbool.h>
#include <sched.h>
#include <signal.h>

#define _BLE_INTERNAL_
#include "./battery_level_engine.h"

#ifndef NDEBUG
# include <stdio.h>
#endif

#define BLE_END_LINE "\r\n\r\nOK"

#define _ble_send(ble, hdv_data) __internal_ble_send(ble->sock, "\r\n" hdv_data "\r\n", sizeof(hdv_data) + 4, 0)
#define _ble_recv(ble, hdv_data) __internal_ble_recv(ble->sock, hdv_data, BUFFER_SIZE, 0)

#define _CALL_EVENT_HANDLER(function_type, event_name, ...) { \
  if (!!ble->event_handlers[event_name].handler) {            \
    ((function_type)ble->event_handlers[event_name].handler)( \
      __VA_ARGS__, ble->event_handlers[event_name].data       \
    );                                                        \
  }                                                           \
}

#define _CALL_EVENT_HANDLER_WITH_RETURN(function_type, ret, event_name, ...) {  \
  if (!!ble->event_handlers[event_name].handler) {                              \
    ret = ((function_type)ble->event_handlers[event_name].handler)(             \
      __VA_ARGS__, ble->event_handlers[event_name].data                         \
    );                                                                          \
  }                                                                             \
}

#ifndef NDEBUG

ssize_t __internal_ble_send(int __fd, const void *__buf, size_t __n, int __flags) {
  fprintf(stderr, "[  TO DEVICE] %.*s\n", (int)__n, (char*)__buf);
  return send(__fd, __buf, __n, __flags);
}

ssize_t __internal_ble_recv(int __fd, void *__buf, size_t __n, int __flags) {
  ssize_t len = recv(__fd, __buf, __n, __flags);

  fprintf(stderr, "[FROM DEVICE] %.*s\n", (int)len, (char*)__buf);
  return len;
}

#else

#define __internal_ble_send send
#define __internal_ble_recv recv

#endif

static io_context_t*    _sdp_ctx = NULL;
static pthread_t        _sdp_thread = 0;
static bool             _sdp_thread_stop = false;

static void* _sdp_routine() {
  sigset_t  mask;
  
  sigfillset(&mask);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);

  int ctx_res;
  while ((ctx_res = ioc_wait_once(_sdp_ctx)) == 0) {
    if (_sdp_thread_stop) {
      break;
    }
  }

  delete_io_context(_sdp_ctx);

  return NULL;
}

__attribute__((constructor)) static void _create_sdp_thread() {
  _sdp_ctx = create_io_context();
  if (pthread_create(&_sdp_thread, NULL, (void*)_sdp_routine, NULL)) { 
    perror("pthread_create"); 
    exit(EXIT_FAILURE); 
  }
  sched_yield(); // force the new thread to run now by putting main thread to the back of the run stack
}

__attribute__((destructor)) static void _cleanup_sdp_thread() {
  _sdp_thread_stop = true;
  ioc_stop_wait(_sdp_ctx);
  pthread_join(_sdp_thread, NULL);
}

ble_t* create_battery_level_engine(io_context_t* ctx) {
  ble_t* ble = calloc(1, sizeof(ble_t));

  if (!ble) {
    ble->ble_error_message = strerror(errno);
  }
  ble->ctx = ctx;
  return ble;
}

int   delete_battery_level_engine(ble_t* ble) {
  if (!!ble->ioc_sock_handle) {
    ioc_remove_handle(ble->ioc_sock_handle);
  }
  
  if (ble->is_connected) {
    if (close(ble->sock) < 0) {
      ble->ble_error_message = strerror(errno);
      return -1;
    }
  }

  free(ble);
  return 0;
}

struct __data_find_channel_result {
  ble_t*      ble;
  const char* addr;
  bool        success;
  uint8_t     channel;
};

struct __data_find_channel {
  ble_t*      ble;
  const char* addr;
};

static void _internal_ble_find_rfcomm_channel_ended(struct __data_find_channel_result* res) {
  const ble_t* ble = res->ble;

  _CALL_EVENT_HANDLER(
    ble_on_rfcomm_channel_find_ended_handler_t,
    BLE_RFCOMM_CHANNEL_FOUND,
    res->ble, res->success, res->addr, res->channel 
  );
}

static void _internal_ble_find_rfcomm_channel(struct __data_find_channel* data) {
  ble_t*      ble = data->ble;
  const char* addr = data->addr;

  free(data);

  bdaddr_t                            target;
  sdp_list_t                          *attrid, *search, *seq, *next;
  sdp_session_t*                      session = 0;
  uint32_t                            range = 0x0000ffff;
  uuid_t                              group = { 0 };
  uint8_t                             port = 0;
  struct __data_find_channel_result*  res = malloc(sizeof(*res));

  if (!res) {
    perror("malloc()");
    exit(EXIT_FAILURE);
  }

  res->ble = ble;
  res->addr = addr;

  str2ba(addr, &target);
  sdp_uuid16_create(&group, HANDSFREE_SVCLASS_ID);

  session = sdp_connect(BDADDR_ANY, &target, SDP_RETRY_IF_BUSY);

  if (!session) {
    ble->ble_error_message = strerror(errno);
    res->success = false;
    ioc_set_handle_delete_func(
      ioc_timeout(ble->ctx, 0, (ioc_timeout_func_t)&_internal_ble_find_rfcomm_channel_ended, res),
      &free
    );
    return;
  }

  attrid = sdp_list_append(0, &range);
  search = sdp_list_append(0, &group);
  if (sdp_service_search_attr_req(session, search, SDP_ATTR_REQ_RANGE, attrid, &seq)) {
    ble->ble_error_message = strerror(errno);
    sdp_close(session);
    res->success = false;
    ioc_set_handle_delete_func(
      ioc_timeout(ble->ctx, 0, (ioc_timeout_func_t)&_internal_ble_find_rfcomm_channel_ended, res),
      &free
    );
    return;
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

    res->channel = port;
    res->success = true;
  } else {
    ble->ble_error_message = "Device not compatible: no hand free channel found";
    res->success = false;
  }
  ioc_set_handle_delete_func(
    ioc_timeout(ble->ctx, 0, (ioc_timeout_func_t)&_internal_ble_find_rfcomm_channel_ended, res),
    &free
  );
}

// result will be given by registring event: ble_register_event_handler(ioc, BLE_RFCOMM_CHANNEL_FOUND, &your_callback)
int ble_find_rfcomm_channel(ble_t *ble, const char* addr) {
  struct __data_find_channel* data = malloc(sizeof(*data));

  if (!data) {
    ble->ble_error_message = "Out of memory";
    return -1;
  }

  data->ble = ble;
  data->addr = addr;

  // call _internal_ble_find_rfcomm_channel inside the _sdp_ctx that is running in an other thread
  //   that can "simulate" an async method
  ioc_timeout(_sdp_ctx, 0, (ioc_timeout_func_t)&_internal_ble_find_rfcomm_channel, data);
  return 0;
}

static void  _ble_on_socket_data_is_pending(int fd, uint32_t event, ble_t* ble) {
  (void)event;
  (void)fd;

  if (ble->is_connected) {
    ssize_t recv_len = 0;

    while ((recv_len = _ble_recv(ble, ble->buffer)) >= 0) {
      if (_ble_strstr(ble, "BRSF", recv_len)) {
        _ble_send(ble, "+BRSF:20"BLE_END_LINE);
      } else if(_ble_strstr(ble, "CIND=", recv_len)) {
        _ble_send(ble, "+CIND: (\"battchg\",(0-5)),(\"signal\",(0-5))(\"service\",(0,1)),(\"call\",(0,1)),(\"callsetup\",(0-3)),(\"callheld\",(0-2)),(\"roam\",(0,1))"BLE_END_LINE);
      } else if (_ble_strstr(ble, "CIND?", recv_len)) {
        _ble_send(ble, "+CIND: 5,5,1,0,0,0,0"BLE_END_LINE);
      } else if (_ble_strstr(ble, "AT+XAPL", recv_len)) {
        _ble_send(ble, "+XAPL=iPhone,7"BLE_END_LINE); // lel
      } else if (_ble_strstr(ble, "AT+APLSIRI?", recv_len)) {
        _ble_send(ble, "+APLSIRI=0"BLE_END_LINE);
        ble->ready = true;
        _CALL_EVENT_HANDLER(ble_on_ready_handler_t, BLE_READY, ble);
      } else if (_ble_strstr(ble, "IPHONEACCEV", recv_len)) {
        _ble_send(ble, "OK");
        if (!_ble_strstr(ble, ",", recv_len)) { continue; }
        ble->buffer[recv_len] = '\0';
        
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
              _CALL_EVENT_HANDLER(ble_on_level_change_handler_t, BLE_BATTERY_LEVEL, ble, (value + 1) * 10);
              return;
            }
          }
          free_split(commands);
        }
      } else {
        _ble_send(ble, "OK");
      }
    }

    ble->ble_error_message = strerror(errno);
    _CALL_EVENT_HANDLER(ble_on_error_handler_t, BLE_ERROR, ble, ble->ble_error_message);
  } else {
    ble->ble_error_message = "Not connect to a device: call ble_connect_to() before";
    _CALL_EVENT_HANDLER(ble_on_error_handler_t, BLE_ERROR, ble, ble->ble_error_message);
  }
}

void  ble_register_event_handler(ble_t* ble, enum ble_events_e e, void* event_handler, void* data) {
  ble->event_handlers[e] = (typeof(*ble->event_handlers)) { .handler = event_handler, .data = data };
}

int ble_connect_to(ble_t* ble, const char* addr, uint8_t channel) {
  #define RFCOMM_CHANNEL_MAX 30

  channel = channel ? channel : ble->channel;
  
  if (channel == 0) {
    ble->ble_error_message = "No channel found, please run 'ble_find_rfcomm_channel()' before";
    return -1;
  }

  bool  retry = channel <= RFCOMM_CHANNEL_MAX; 
  if (!retry) {
    channel -= RFCOMM_CHANNEL_MAX;
  }

  ble->sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  if (ble->sock < 0) {
    ble->ble_error_message = strerror(errno);
    return -1;
  }
  ble->rem_addr.rc_family = AF_BLUETOOTH;
  ble->rem_addr.rc_channel = (uint8_t) channel;
  str2ba(addr, &ble->rem_addr.rc_bdaddr);

  int   connect_res;

  connect_res = connect(ble->sock, (struct sockaddr *)&ble->rem_addr, sizeof(ble->rem_addr));

  if (connect_res < 0) {
    close(ble->sock);

    if (errno == ECONNREFUSED && retry) {
#     if !defined(NDEBUG)
        fprintf(stderr, "Level_engine: connection refused, retrying (%d)...\n", retry);
#     endif
      return ble_connect_to(ble, addr, channel + RFCOMM_CHANNEL_MAX);
    }
    ble->ble_error_message = strerror(errno);
    return connect_res;
  }

  ble->ioc_sock_handle = ioc_add_fd(ble->ctx, ble->sock, EPOLLIN, (ioc_event_func_t)&_ble_on_socket_data_is_pending, ble);

  ble->is_connected = 1;
  return 0;
}

int ble_send_command(ble_t* ble, const char* cmd) {
  if (!ble->is_connected) {
    ble->ble_error_message = "Not connect to a device: call ble_connect_to() before";
    return -1;
  }
  if (!ble->ready) {
    ble->ble_error_message = "You must wait the event READY before";
    return -1;
  }

  size_t  cmd_len = strlen(cmd);
  char*   str = malloc(cmd_len + sizeof(BLE_END_LINE) + 4);

  if (!str) {
    ble->ble_error_message = "Out of memory";
    return -1;
  }

  str[0] = '\r';
  str[1] = '\n';
  memcpy(str + 2, cmd, cmd_len);
  memcpy(str + 2 + cmd_len, "\r\n\r\nOK\r\n", sizeof("\r\n\r\nOK\r\n"));

  __internal_ble_send(ble->sock, str, cmd_len + sizeof(BLE_END_LINE) + 4, 0);

  free(str);
  return 0;
}

const char* ble_get_last_error_message(ble_t* ble) {
  return ble->ble_error_message;
}