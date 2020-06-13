#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../srcs/io_context.h"

static void on_stdin_data_pending(int fd, uint32_t event, ioc_data_t data);

typedef struct io_data_s {
  char buffer[1024];
  char stop;
} io_data_t;

int main() {
  io_context_t* ctx = create_io_context();
  io_data_t     data = { 0 };

  printf("%p\n", &data);

  ioc_add_fd(ctx, STDIN_FILENO, 0, on_stdin_data_pending, &data);

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

  delete_io_context(ctx);

  return 0;
}

static void on_stdin_data_pending(int fd, uint32_t event, ioc_data_t data) {
  printf("%p\n", data);
  (void)event;
  char* buffer = ((io_data_t*)data)->buffer;

  ssize_t len = read(fd, buffer, 1024);
  
  if (buffer[len-1] == '\n') {
    --len;
  }

  buffer[len] = 0;

  if (strcmp(buffer, "stop") == 0) {
    ((io_data_t*)data)->stop = 1;
  } else {
    printf("stdin: %s\n", buffer);
  }
}