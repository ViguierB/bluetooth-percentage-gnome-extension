const Main = imports.ui.main;
const GLib = imports.gi.GLib;
const St = imports.gi.St;
const GnomeBluetooth = imports.gi.GnomeBluetooth;

const Me = imports.misc.extensionUtils.getCurrentExtension();
const { bluetooth_battery_level_engine } = Me.imports.srcs.battery_level_engine;
const { signals } = Me.imports.srcs.misc.signals;

class _bluetooth_battery_level_extention extends signals {
  constructor() {
    super();
    this._bluetooth_indicator = Main.panel.statusArea.aggregateMenu._bluetooth;
    this._menu = this._bluetooth_indicator._item.menu;
    this._bt_level = new bluetooth_battery_level_engine(new GnomeBluetooth.Client());
    this._next_loop_handle = null;
  }

  async refresh() {
    log(['Bluetooth percentage: refreshing']);
    const devices = this._bt_level.get_devices();
    const connected_devices_number = this._bt_level.count_connected_devices(devices)

    if (connected_devices_number > 1 || connected_devices_number < 1) {
      return;
    }
    await Promise.all(devices.map(async d => {
      if (d.is_connected) {
        let battery_level = await this._bt_level.get_battery_level(d.mac);

        if (Number.isNaN(battery_level)) {
          this._bluetooth_indicator._item.label.text = `${d.name}`;
          return new Promise((resolve) => {
            GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 2, async () => {r
              await this.refresh();
              resolve();
            });
          });
        }

        this._bluetooth_indicator._item.label.text = `${d.name} (${battery_level} %)`;
      }
    }));
  }

  async loop() {
    try {
      await this.refresh();
    } catch (e) {
      error(e);
    }
    this._next_loop_handle = GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 180, () => {
      this.loop();
    });
  }

  _register_signals() {
    this.register_signal(this._bt_level, 'device-inserted', (_ctrl, _device) => {
      if (this._next_loop_handle !== null) {
        GLib.source_remove(this._next_loop_handle);
      }
      GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 5, () => {
        this.loop()
      });
    });
    this.register_signal(this._bt_level, 'device-changed', (_ctrl, _device) => {
      if (this._next_loop_handle !== null) {
        GLib.source_remove(this._next_loop_handle);
      }
      GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 5, () => {
        this.loop()
      });
    });
    this.register_signal(this._bt_level, 'device-deleted', () => {
      if (this._next_loop_handle !== null) {
        GLib.source_remove(this._next_loop_handle);
      }
      GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 5, () => {
        this.loop()
      });
    });
  }

  _unregister_signals() {
    this.unregister_signal_all();
  }

  enable() {
    // make sure GnomeBluetooth is ready
    this._register_signals();
    this._bt_level.enable();
    GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 0, () => {
      this.loop()
    });
  }

  disable() {
    this._bt_level.disable();
    this._unregister_signals();
    if (this._next_loop_handle !== null) {
      GLib.source_remove(this._next_loop_handle);
    }
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;