#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "battery_level_engine.h"
#include "connection_manager.h"
#include "internal.h"

struct exec_options {
  char*             cmd;
  struct ctx_data*  data;
};

// static void  _cmd_cmd(struct exec_options* opts, const char* rest);
static void  _cmd_timeout(struct exec_options* opt, const char* rest);
static void  _cmd_stop(struct exec_options* opt, const char* rest);
static void  _cmd_connect_to(struct exec_options* opt, const char* rest);
static void  _cmd_list_active_connections(struct exec_options* opt, const char* rest);

static struct {
  struct {
    enum {
      COMMAND_KEY_STRING,
      COMMAND_KEY_FUNCTION
    }           type;
    union {
      const char* skey;
      const char* (*fct)(const char* command);
    }           value;
  }           key;
  void        (*executor)(struct exec_options* opts, const char* rest);
} const _commands[] = {
  // { .key.type = COMMAND_KEY_STRING, .key.value.skey = "cmd", .executor = &_cmd_cmd },
  { .key.type = COMMAND_KEY_STRING, .key.value.skey = "timeout", .executor = &_cmd_timeout },
  { .key.type = COMMAND_KEY_STRING, .key.value.skey = "stop", .executor = &_cmd_stop },
  { .key.type = COMMAND_KEY_STRING, .key.value.skey = "connect-to", .executor = &_cmd_connect_to },
  { .key.type = COMMAND_KEY_STRING, .key.value.skey = "list-active-connections", .executor = &_cmd_list_active_connections },
};

void  free_exec_options(struct exec_options* opts) {
  free(opts->cmd);
  free(opts);
}

static inline void exec_command(struct exec_options* opt) {
  for (uint32_t i = 0; i < sizeof(_commands) / sizeof(*_commands); ++i) {
    typeof(*_commands)* cur = _commands + i;

    switch (cur->key.type) {
      case COMMAND_KEY_STRING:
        if (strstr(opt->cmd, cur->key.value.skey) == opt->cmd) {
          cur->executor(opt, opt->cmd + strlen(cur->key.value.skey));
        }
        break;
      case COMMAND_KEY_FUNCTION:
        {
          const char* rest = NULL;
          if (!!(rest = cur->key.value.fct(opt->cmd))) {
            cur->executor(opt, rest);
          }
        }
        break;
      default:
        assert(false); // malformed command function array or corrupted memory
        break;
    }
  }
}

void on_stdin_data_pending(int fd, uint32_t event, struct ctx_data* data) {
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

// static void  _cmd_cmd(struct exec_options* opt, const char* rest) {
//   ble_send_command(opt->data->ble, opt->cmd + sizeof("cmd"));
// }

static void  _cmd_stop(struct exec_options* opt, const char* params) {
  (void)params;
  opt->data->stop = 1;
}

static void  _cmd_connect_to(struct exec_options* opt, const char* params) {
  connection_t* co = cm_add_connection(opt->data->cm, params);

  cm_connection_connect(co);
}

static void _cmd_list_active_connections_iter_function(connection_t* connection, FILE* stream) {
  fprintf(stream, "\"%s\",", cm_connection_get_addr(connection));
}

static void  _cmd_list_active_connections(struct exec_options* opt, const char* params){
  (void)params;

  char*   response;
  size_t  response_len;
  FILE*   stream = open_memstream(&response, &response_len);

  cm_foreach_connections(opt->data->cm, (void*)&_cmd_list_active_connections_iter_function, stream);
  fclose(stream);

  if (response_len > 0) {
    printf("{\"devices\":[%.*s]}\n", (int) response_len - 1, response);
  } else {
    printf("{\"devices\":[]}\n");
  }
  free(response);
}

static void  _cmd_timeout(struct exec_options* opt, const char* params) {
  struct exec_options*  o = malloc(sizeof(struct exec_options));
  char                  str[1024];
  int                   timeout;

  sscanf(params, "%d %1024[^\n]", &timeout, str);

  o->cmd = strdup(str);
  o->data = opt->data;
  ioc_handle_t* h = ioc_timeout(opt->data->ctx, timeout * 1000, (ioc_timeout_func_t)&exec_command, o);
  ioc_set_handle_delete_func(h, (ioc_data_free_func_t)&free_exec_options);
}