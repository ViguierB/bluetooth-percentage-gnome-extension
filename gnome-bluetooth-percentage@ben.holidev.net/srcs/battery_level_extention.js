const Me = imports.misc.extensionUtils.getCurrentExtension();
const { bluetooth_battery_level_engine } = Me.imports.srcs.battery_level_engine;

class _bluetooth_battery_level_extention {
  constructor() {
    this.bt_engine = new bluetooth_battery_level_engine();
  }

  enable() {
    this.bt_engine.test_connection();
  }

  disable() {
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;