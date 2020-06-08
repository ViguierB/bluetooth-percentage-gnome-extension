#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "./io_context.h"
#include "./battery_level_engine.h"

static void on_battery_level_change(void* ble, int level);
static void on_ble_error(void* ble, char* error_string);
static void on_stdin_data_pending(uint32_t events, ioc_data_wrap_t* data);

struct ctx_data {
  void* ble;
  int   stop;
};

int main(int ac, char **av) {
  if (ac != 2 ) {
    fprintf(stderr, "bad arg number: expected 1\n");
    return EXIT_FAILURE;
  }

  char*           address = av[1];
  io_context_t*   ctx;
  void*           bl_engine;
  struct ctx_data d = { 0 };

  if (!(ctx = create_io_context())) {
    fprintf(stderr, "create_io_context() failed\n");
    return EXIT_FAILURE;
  }

  if (!(bl_engine = create_battery_level_engine(ctx))) {
    fprintf(stderr, "create_battery_level_engine() failed\n");
    return EXIT_FAILURE;
  }

  ble_register_event_handler(bl_engine, BLE_BATTERY_LEVEL, &on_battery_level_change);
  ble_register_event_handler(bl_engine, BLE_ERROR, &on_ble_error);

  if (ble_find_rfcomm_channel(bl_engine, address) != 0) {
    fprintf(stderr, "Failed to connect to SDP server on %s: %s\n", address, ble_get_last_error_message(bl_engine));
    return EXIT_FAILURE;
  }


  if (ble_connect_to(bl_engine, address) < 0) {
    fprintf(stderr, "unable to connect to this bluetooth device: %s\n", ble_get_last_error_message(bl_engine));
    return EXIT_FAILURE;
  }

  if (ble_hfp_nogiciate(bl_engine) < 0) {
    fprintf(stderr, "unable to negociate with this bluetooth device: %s\n", ble_get_last_error_message(bl_engine));
    return EXIT_FAILURE;
  }

  d.ble = bl_engine;
  ioc_handle_t stdin_handle = ioc_add_fd(ctx, STDIN_FILENO, 0, on_stdin_data_pending, &d);

  int ctx_res;
  while ((ctx_res = ioc_wait_once(ctx)) == 0) {
    if (!!d.stop) {
      break;
    }
  }
  if (ctx_res < 0) {
    fprintf(stderr, "%s\n", ioc_get_last_error(ctx));
    return EXIT_FAILURE;
  }

  ioc_remove_fd(ctx, stdin_handle);
  delete_battery_level_engine(bl_engine);
  delete_io_context(ctx);

  return EXIT_SUCCESS;
}

static void on_battery_level_change(void* ble, int level) {
  (void)ble;
  printf("%d\n", level);
  fflush(stdout);
}

static void on_ble_error(void* ble, char* error_string) {
  (void)ble;
  fprintf(stderr, "%s\n", error_string);
  exit(EXIT_FAILURE);
}

static void on_stdin_data_pending(uint32_t event, ioc_data_wrap_t* data) {
  (void)event;
  char buffer[1024];

  ssize_t len = read(data->fd, buffer, 1024);
  
  if (buffer[len-1] == '\n') {
    --len;
  }

  buffer[len] = 0;

  if (strstr(buffer, "cmd") == buffer) {
    ble_send_command(((struct ctx_data*)data->data)->ble, buffer + sizeof("cmd"));
  } else if (strcmp(buffer, "stop") == 0) {
    ((struct ctx_data*)data->data)->stop = 1;
  }
}
