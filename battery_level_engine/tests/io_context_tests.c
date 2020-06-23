#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/epoll.h>
#include <errno.h>

#include "../srcs/io_context.h"

typedef struct io_data_s {
  char          stop;
  io_context_t* ctx;
  char          buffer[1024];
} io_data_t;

static void on_stdin_data_pending(int fd, uint32_t event, io_data_t* data);
static void connect_to_holidev(io_context_t* ctx);

void  useless_func(void*empty) { (void)empty; printf("WTF??\n"); }

struct timeout_remove_test_s {
  io_context_t* ctx;
  ioc_handle_t  hdl;
};

void  remove_timeout_test(struct timeout_remove_test_s* hdl_w) {
  printf("removing timeout\n");
  ioc_remove_handle(hdl_w->hdl);
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
  connect_to_holidev(ctx);


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

  ioc_remove_handle(handle);

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

    sscanf(buffer + sizeof("timeout"), "%d %128[^\n]", &timeout, str);
    ioc_handle_t handle = ioc_timeout(data->ctx, timeout * 1000, (ioc_timeout_func_t)&on_timeout_func, strdup(str));
    ioc_set_handle_delete_func(handle, (ioc_data_free_func_t)&timeout_delete_func);
  } else {
    printf("stdin: %s\n", buffer);
  }
}


typedef int socket_t;
typedef struct connection {
  socket_t      sock;
  char          buf[1024];
  ioc_handle_t  connection_handle;
  ioc_handle_t  timeout_handle;
} connection_t;

static void on_connect_change(int fd, uint32_t events, connection_t* co) {
  printf("%0x %0x %0x %0x\n", events, EPOLLOUT, EPOLLIN, EPOLLERR);
  if (events & (EPOLLRDHUP | EPOLLHUP)) {
    printf("fuck\n");
  }

  char* buffer = co->buf;

  ssize_t len = recv(fd, buffer, 1024, 0);
  
  if (buffer[len-1] == '\n') {
    --len;
  }

  buffer[len] = 0;

  printf("holidev.net: %s\n", buffer);
}

static void on_connect_timeout(connection_t* co) {
  printf("timeout \n");
  close(co->sock);
  ioc_remove_handle(co->connection_handle);
}

static void connect_to_holidev(io_context_t* ctx) {
  const int           yes = 1;
  connection_t*       co = malloc(sizeof(connection_t));
  struct sockaddr_in  serv_addr; 

  if ((co->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket(): %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(8694);

  if(inet_pton(AF_INET, "192.0.0.2", &serv_addr.sin_addr) <= 0) { 
      fprintf(stderr, "Invalid address/ Address not supported \n");
      exit(EXIT_FAILURE);
  }
  if (setsockopt(co->sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
    perror("setsockopt || fcntl");
    exit(1);
  }

  int arg;
  if( (arg = fcntl(co->sock, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL): %s\n", strerror(errno)); 
     exit(0); 
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(co->sock, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL): %s\n", strerror(errno)); 
     exit(0); 
  } 

  if (
    connect(co->sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 &&
    errno != EINPROGRESS
  ) {
    fprintf(stderr, "HUMMMM: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    co->timeout_handle = ioc_timeout(ctx, 5000, (ioc_timeout_func_t)&on_connect_timeout, co);
    co->connection_handle = ioc_add_fd(ctx, co->sock, EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET, (ioc_event_func_t)&on_connect_change, co);
    printf("hannn\n");
  }
  printf("async boiiii\n");
}