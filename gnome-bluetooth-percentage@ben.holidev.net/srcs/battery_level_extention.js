const Main = imports.ui.main;
const GLib = imports.gi.GLib;
const St = imports.gi.St;
const GnomeBluetooth = imports.gi.GnomeBluetooth;

const Me = imports.misc.extensionUtils.getCurrentExtension();
const { bluetooth_battery_level_engine } = Me.imports.srcs.battery_level_engine;

class _bluetooth_battery_level_extention {
  constructor() {
    this._bluetooth_indicator = Main.panel.statusArea.aggregateMenu._bluetooth;
    this._menu = this._bluetooth_indicator._item.menu;
    this._bt_level = new bluetooth_battery_level_engine(new GnomeBluetooth.Client());
    this._next_loop_handle = null;
  }

  async refresh() {
    const devices = this._bt_level.get_devices();
    const connected_devices_number = this._bt_level.count_connected_devices(devices)

    if (connected_devices_number > 1 || connected_devices_number < 1) {
      return;
    }
    await Promise.all(devices.map(async d => {
      if (d.is_connected) {
        let battery_level = await this._bt_level.get_battery_level(d.mac);

        this._bluetooth_indicator._item.label.text += ` (${battery_level} %)`;
      }
    }));
  }

  async loop() {
    try {
      await this.refresh();
    } catch (e) {
      error(e);
    }
    this._next_loop_handle = GLib.timeout_add_seconds(GLib.PRIORITY_LOW, 180, () => {
      this.loop();
    });
  }

  enable() {
    // make sure GnomeBluetooth is ready
    GLib.timeout_add_seconds(GLib.PRIORITY_LOW, 0, () => {
      this.loop()
    });
  }

  disable() {
    if (this._next_loop_handle) {
      GLib.source_remove(this._next_loop_handle);
    }
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;