const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;

class _bluetooth_battery_level_engine {
  constructor() {}

  test_connection() {
    GLib.spawn_command_line_async()

    log("~~~~TEST~~~~");
  }

  get_connected_devices() {
    
  }
}

var bluetooth_battery_level_engine = _bluetooth_battery_level_engine;