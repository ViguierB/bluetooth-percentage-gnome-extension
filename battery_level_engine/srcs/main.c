#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "./battery_level_engine.h"

int main(int ac, char **av) {
  if (ac != 2 ) {
    fprintf(stderr, "bad arg number: expected 1\n");
    return EXIT_FAILURE;
  }

  char*     address = av[1];
  void*     bl_engine;


  bl_engine = create_battery_level_engine();
  
  if (!bl_engine) {
    fprintf(stderr, "create_battery_level_engine() failed\n");
    return EXIT_FAILURE;
  }

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

  int level;
  
  while ((level = ble_get_battery_level(bl_engine)) >= 0) {
    if (level == -1) {
      fprintf(stderr, "%s\n", ble_get_last_error_message(bl_engine));
      return EXIT_FAILURE;
    }

    printf("%d\n", level);
    fflush(stdout);
  }


  delete_battery_level_engine(bl_engine);

  return EXIT_SUCCESS;
}
