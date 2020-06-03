#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

void*       create_battery_level_engine();
int         delete_battery_level_engine(void*);
int         ble_connect_to(void*, char* addr, uint16_t channel);
int         ble_get_battery_level(void*);
const char* ble_get_last_error_message();

int main(int ac, char **av) {
  if (ac != 3 ) {
    fprintf(stderr, "bad arg number: expected 3\n");
    return EXIT_FAILURE;
  }

  char*     address = av[1];
  char*     channel_str_error_handle;
  uint16_t  channel = strtol(av[2], &channel_str_error_handle, 10);
  void*     bl_engine;

  if (*channel_str_error_handle != '\0') {
    fprintf(stderr, "Bad given channel\n");
    return EXIT_FAILURE;
  }

  bl_engine = create_battery_level_engine();

  if (!bl_engine) {
    fprintf(stderr, "create_battery_level_engine() failed\n");
    return EXIT_FAILURE;
  }

  if (ble_connect_to(bl_engine, address, channel) < 0) {
    fprintf(stderr, "unable to connect to this bleutooth device: %s\n", ble_get_last_error_message());
    return EXIT_FAILURE;
  }

  int level = ble_get_battery_level(bl_engine);

  if (level == -1) {
    fprintf(stderr, "%s\n", ble_get_last_error_message());
    return EXIT_FAILURE;
  }

  printf("%d\n", level);

  delete_battery_level_engine(bl_engine);

  return EXIT_SUCCESS;
}