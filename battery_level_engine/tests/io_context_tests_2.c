#include <stdio.h>
#include <stdlib.h>

#include "../srcs/io_context.h"

int main() {
  io_context_t* ctx = create_io_context();

  int ctx_res;
  while ((ctx_res = ioc_wait_once(ctx)) == 0) {
    // must wait indefinitly
    printf("Test\n");
  }

  if (ctx_res < 0) {
    fprintf(stderr, "%s\n", ioc_get_last_error(ctx));
    return EXIT_FAILURE;
  }


  delete_io_context(ctx);

  return 0;
}