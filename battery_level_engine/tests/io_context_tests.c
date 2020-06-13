#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../srcs/io_context.h"

typedef struct io_data_s {
  char          stop;
  io_context_t* ctx;
  char          buffer[1024];
} io_data_t;

static void on_stdin_data_pending(int fd, uint32_t event, io_data_t* data);

void  useless_func(void*empty) { (void)empty; printf("WTF??\n"); }

struct timeout_remove_test_s {
  io_context_t* ctx;
  ioc_handle_t  hdl;
};

void  remove_timeout_test(struct timeout_remove_test_s* hdl_w) {
  printf("removing timeout\n");
  ioc_remove_handle(hdl_w->ctx, hdl_w->hdl);
}

void  test_timeout(io_context_t* ctx) {
  struct timeout_remove_test_s* h = malloc(sizeof(struct timeout_remove_test_s));

  h->hdl = ioc_timeout(ctx, 2000, (void*)&useless_func, NULL);
  h->ctx = ctx;
  ioc_handle_t rh = ioc_timeout(ctx, 1000, (ioc_timeout_func_t)&remove_timeout_test, h);
  ioc_set_handle_delete_func(rh, (ioc_data_free_func_t)&free);
}

int main() {
  io_context_t* ctx = create_io_context();
  io_data_t     data = { 0 };

  data.ctx = ctx;
  ioc_handle_t* handle = ioc_add_fd(ctx, STDIN_FILENO, 0, (ioc_event_func_t)&on_stdin_data_pending, &data);

  int ctx_res;
  while ((ctx_res = ioc_wait_once(ctx)) == 0) {
    if (!!data.stop) {
      break;
    }
  }
  if (ctx_res < 0) {
    fprintf(stderr, "%s\n", ioc_get_last_error(ctx));
    return EXIT_FAILURE;
  }

  ioc_remove_handle(ctx, handle);

  delete_io_context(ctx);

  return 0;
}

static void on_timeout_func(char* str) {
  printf("TIMEOUT: %s\n", str);
}

static void timeout_delete_func(char* str) {
  free(str);
}

static void on_stdin_data_pending(int fd, uint32_t event, io_data_t* data) {
  (void)event;
  char* buffer = ((io_data_t*)data)->buffer;

  ssize_t len = read(fd, buffer, 1024);
  
  if (buffer[len-1] == '\n') {
    --len;
  }

  buffer[len] = 0;

  if (strcmp(buffer, "stop") == 0) {
    ((io_data_t*)data)->stop = 1;
  } else if (strcmp(buffer, "test_timeout") == 0) {
    test_timeout(data->ctx);
  } else if (strncmp(buffer, "timeout", sizeof("timeout") - 1) == 0) {
    int   timeout;
    char  str[128];

    sscanf(buffer + sizeof("timeout"), "%d %24[^\n]", &timeout, str);
    ioc_handle_t handle = ioc_timeout(data->ctx, timeout * 1000, (ioc_timeout_func_t)&on_timeout_func, strdup(str));
    ioc_set_handle_delete_func(handle, (ioc_data_free_func_t)&timeout_delete_func);
  } else {
    printf("stdin: %s\n", buffer);
  }
}