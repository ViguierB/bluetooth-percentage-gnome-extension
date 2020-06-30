#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "./io_context.h"
#include "./signals.h"
#include "./battery_level_engine.h"

struct ctx_data {
  void*         ble;
  io_context_t* ctx;
  int           stop;
  char          buffer[1024];
};

static void on_battery_level_change(void* ble, int level);
static void on_ble_error(void* ble, char* error_string);
static void on_stdin_data_pending(int fd, uint32_t events, struct ctx_data* data);
static void register_signals(signals_t* s, struct ctx_data* data);


int main(int ac, char **av) {
  if (ac != 2 ) {
    fprintf(stderr, "bad arg number: expected 1\n");
    return EXIT_FAILURE;
  }

  char*           address = av[1];
  io_context_t*   ctx;
  signals_t*      signals;
  void*           bl_engine;
  struct ctx_data d = { 0 };

  if (!(ctx = create_io_context())) {
    fprintf(stderr, "create_io_context() failed\n");
    return EXIT_FAILURE;
  }

  if (!(signals = signal_create(ctx))) {
    fprintf(stderr, "signal_create() failed\n");
    return EXIT_FAILURE;
  } else {
    register_signals(signals, &d);
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
    fprintf(stderr, "unable to connect to %s: %s\n", address, ble_get_last_error_message(bl_engine));
    return EXIT_FAILURE;
  }

  if (ble_hfp_nogiciate(bl_engine) < 0) {
    fprintf(stderr, "unable to negociate with %s: %s\n", address, ble_get_last_error_message(bl_engine));
    return EXIT_FAILURE;
  }

  d.ctx = ctx;
  d.ble = bl_engine;
  ioc_add_fd(ctx, STDIN_FILENO, 0, (ioc_event_func_t)&on_stdin_data_pending, &d);

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

  delete_battery_level_engine(bl_engine);
  signal_delete(signals);
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

struct exec_options {
  char*             cmd;
  struct ctx_data*  data;
};

void  free_exec_options(struct exec_options* opts) {
  free(opts->cmd);
  free(opts);
}

static inline void exec_command(struct exec_options* opt) {
  if (strstr(opt->cmd, "cmd") == opt->cmd) {
    ble_send_command(opt->data->ble, opt->cmd + sizeof("cmd"));
  } else if (strstr(opt->cmd, "timeout") == opt->cmd) {
    struct exec_options*  o = malloc(sizeof(struct exec_options));
    char                  str[1024];
    int                   timeout;

    sscanf(opt->cmd + sizeof("timeout"), "%d %1024[^\n]", &timeout, str);

    o->cmd = strdup(str);
    o->data = opt->data;
    ioc_handle_t h = ioc_timeout(opt->data->ctx, timeout * 1000, (ioc_timeout_func_t)&exec_command, o);
    ioc_set_handle_delete_func(h, (ioc_data_free_func_t)&free_exec_options);
  } else if (strcmp(opt->cmd, "stop") == 0) {
    opt->data->stop = 1;
  }
}

static void on_stdin_data_pending(int fd, uint32_t event, struct ctx_data* data) {
  (void)event;
  char* buffer = data->buffer;

  ssize_t len = read(fd, buffer, 1024);
  
  if (buffer[len-1] == '\n') {
    --len;
  }

  buffer[len] = 0;

  struct exec_options o = {
    .cmd = buffer,
    .data = data
  };

  exec_command(&o);
}

static void _signal_handler(struct ctx_data* data) {
  printf("closing...\n");
  fflush(stdout);
  data->stop = 1;
}

static void register_signals(signals_t* s, struct ctx_data* data) {
  signal_set_t* set = signal_create_set(
    MAKE_SIGNAL_SET_ELEM(SIGINT, &_signal_handler, data),
    MAKE_SIGNAL_SET_ELEM(SIGTERM, &_signal_handler, data)
  );

  if (!set) {
    fprintf(stderr, "%s\n", strerror(errno));
  }

  if (signal_set(s, set) < 0) {
    fprintf(stderr, "%s\n", strerror(errno));
  }
}