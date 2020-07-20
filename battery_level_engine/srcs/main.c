#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include "io_context.h"
#include "signals.h"
#include "battery_level_engine.h"
#include "internal.h"
#include "connection_manager.h"

// static void on_battery_level_change(void* ble, int level);
// static void on_ble_error(void* ble, char* error_string);
// static void on_ble_rfcomm_channel_found(void* ble, bool success, const char* address, uint8_t channel);
static void register_signals(signals_t* s, struct ctx_data* data);

int main(/* int ac, char **av */) {
  // if (ac != 2 ) {
  //   fprintf(stderr, "bad arg number: expected 1\n");
  //   return EXIT_FAILURE;
  // }

  // char*           address = av[1];
  io_context_t*   ctx;
  signals_t*      signals;
  cm_t*           manager;
  // void*           bl_engine;
  struct ctx_data d = { 0 };

  if (!(ctx = create_io_context())) {
    fprintf(stderr, "create_io_context() failed\n");
    return EXIT_FAILURE;
  }

  if (!(manager = cm_create(ctx))) {
    fprintf(stderr, "cm_create() failed\n");
    return EXIT_FAILURE;
  }

  if (!(signals = signal_create(ctx))) {
    fprintf(stderr, "signal_create() failed\n");
    return EXIT_FAILURE;
  } else {
    register_signals(signals, &d);
  }

  // if (!(bl_engine = create_battery_level_engine(ctx))) {
  //   fprintf(stderr, "create_battery_level_engine() failed\n");
  //   return EXIT_FAILURE;
  // }

  // ble_register_event_handler(bl_engine, BLE_RFCOMM_CHANNEL_FOUND, &on_ble_rfcomm_channel_found, NULL);
  // ble_register_event_handler(bl_engine, BLE_BATTERY_LEVEL, &on_battery_level_change, NULL);
  // ble_register_event_handler(bl_engine, BLE_ERROR, &on_ble_error, NULL);

  // if (ble_find_rfcomm_channel(bl_engine, address) != 0) {
  //   fprintf(stderr, "Failed to connect to SDP server on %s: %s\n", address, ble_get_last_error_message(bl_engine));
  //   return EXIT_FAILURE;
  // }

  d.ctx = ctx;
  d.cm = manager;
  // d.ble = bl_engine;
  ioc_add_fd(ctx, STDIN_FILENO, EPOLLIN, (ioc_event_func_t)&on_stdin_data_pending, &d);

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

# ifndef NDEBUG
    fprintf(stderr, "closing...\n");
    fflush(stderr);
# endif

  // delete_battery_level_engine(bl_engine);
  cm_delete(manager);
  signal_delete(signals);
  delete_io_context(ctx);

  return EXIT_SUCCESS;
}

// static void on_ble_rfcomm_channel_found(void* ble, bool success, const char* address, uint8_t channel) {
//   if (!success) {
//     fprintf(stderr, "Failed to connect to SDP server on %s: %s\n", address, ble_get_last_error_message(ble));
//     exit(EXIT_FAILURE);
//   }
//   if (ble_connect_to(ble, address, channel) < 0) {
//     fprintf(stderr, "unable to connect to %s: %s\n", address, ble_get_last_error_message(ble));
//     exit(EXIT_FAILURE);
//   }
// }

// static void on_battery_level_change(void* ble, int level) {
//   (void)ble;
//   printf("%d\n", level);
//   fflush(stdout);
// }

// static void on_ble_error(void* ble, char* error_string) {
//   (void)ble;
//   fprintf(stderr, "%s\n", error_string);
//   exit(EXIT_FAILURE);
// }

static void _signal_handler(struct ctx_data* data) {
  data->stop = 1;
}

static void register_signals(signals_t* s, struct ctx_data* data) {
  signal_set_t* set = signal_create_set(
#   if defined(NDEBUG)
    { SIGINT, &_signal_handler, data },
    { SIGTERM, &_signal_handler, data },
#   endif
    { SIGUSR1, &_signal_handler, data },
    { SIGUSR2, &_signal_handler, data }
  );

  if (!set) {
    fprintf(stderr, "%s\n", strerror(errno));
  }

  if (signal_set(s, set) < 0) {
    fprintf(stderr, "%s\n", strerror(errno));
  }
}