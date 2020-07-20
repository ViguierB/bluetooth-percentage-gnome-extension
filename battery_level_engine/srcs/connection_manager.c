#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define __CM_INTERNAL__
#include "internal.h"
#include "battery_level_engine.h"
#include "circular_double_linked_list.h"

typedef struct connection_s {
  struct connection_s*  prev;
  struct connection_s*  next;

  const char*           addr;
  ble_t*                ble;
  void*                 manager;
}   connection_t;

typedef struct cm_s {
  io_context_t* ctx;
  connection_t* devices;
}   cm_t;

DECLARE_CIRCULAR_DOUBLE_LINKED_LIST(connection_t);

cm_t* cm_create(io_context_t* ctx) {
  cm_t* cm = malloc(sizeof(*cm));

  if (cm == NULL) {
    return NULL;
  }
  cm->ctx = ctx;
  cm->devices = NULL;
  return cm;
}

void  cm_delete(cm_t* cm) {
  if (!cm) { return; }

  connection_t* start;
  connection_t* cur;

  start = cur = cm->devices;

  if (start) {
    do {
      void* delete_later = cur;
      if (!!cur->ble) {
        delete_battery_level_engine(cur->ble);
      }
      free((void*)cur->addr);

      cur = cur->next;
      free(delete_later);
    } while(cur != start);
  }
  free(cm);
}

static void _on_ble_rfcomm_channel_found(void* ble, bool success, const char* address, uint8_t channel, connection_t* connection) {
  (void)connection;

  if (!success) {
    fprintf(stderr, "Failed to connect to SDP server on %s: %s\n", address, ble_get_last_error_message(ble));
    exit(EXIT_FAILURE);
  }
  if (ble_connect_to(ble, address, channel) < 0) {
    fprintf(stderr, "unable to connect to %s: %s\n", address, ble_get_last_error_message(ble));
    exit(EXIT_FAILURE);
  }
}

static void _on_battery_level_change(void* ble, int level, connection_t* connection) {
  (void)ble;
  printf("{\"device\":\"%s\",\"battery_level\": %d}\n", connection->addr, level);
  fflush(stdout);
}

static void _on_ble_error(void* ble, char* error_string, connection_t* connection) {
  (void)ble;
  fprintf(stderr, "[%s] %s\n", connection->addr, error_string);
}

connection_t* cm_add_connection(cm_t* this, const char* addr) {
  connection_t* connection = malloc(sizeof(*connection));

  *connection = (connection_t) {
    .next = connection,
    .prev = connection,
    .addr = str_trim_and_dup(addr),
    .manager = this,
    .ble = create_battery_level_engine(this->ctx)
  };

  ble_register_event_handler(connection->ble, BLE_RFCOMM_CHANNEL_FOUND, &_on_ble_rfcomm_channel_found, connection);
  ble_register_event_handler(connection->ble, BLE_BATTERY_LEVEL, &_on_battery_level_change, connection);
  ble_register_event_handler(connection->ble, BLE_ERROR, &_on_ble_error, connection);
  
  if (!!this->devices) {
    this->devices = list_connection_t_add_before(this->devices, connection);
  } else {
    this->devices = connection;
  }

  return connection;
}

cm_t* cm_connection_get_manager(connection_t* connection) {
  return connection->manager;
}

const char* cm_connection_get_addr(connection_t* connection) {
  return connection->addr;
}

void  cm_foreach_connections(cm_t* this, void (*h)(connection_t* connection, void *data), void* data) {
  connection_t* first;
  connection_t* cur = first = this->devices;

  if (first) {
    do {
      h(cur, data);
      cur = cur->next;
    } while (first != cur);
  }
}

void  cm_connection_connect(connection_t* connection) {
  if (ble_find_rfcomm_channel(connection->ble, connection->addr) != 0) {
    fprintf(stderr, "Failed to connect to SDP server on %s: %s\n", connection->addr, ble_get_last_error_message(connection->ble));
  }
}

