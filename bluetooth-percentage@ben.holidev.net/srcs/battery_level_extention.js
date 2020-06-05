const Main = imports.ui.main;
const GLib = imports.gi.GLib;
const St = imports.gi.St;
const GnomeBluetooth = imports.gi.GnomeBluetooth;

const Me = imports.misc.extensionUtils.getCurrentExtension();
const { bluetooth_battery_level_engine } = Me.imports.srcs.battery_level_engine;
const { signals } = Me.imports.misc;
const { logger } = Me.imports.misc;
const { event_trigger_attenuator } = Me.imports.misc;

class _bluetooth_battery_level_extention extends signals {
  constructor() {
    super();
    this._bluetooth_indicator = Main.panel.statusArea.aggregateMenu._bluetooth;
    this._menu = this._bluetooth_indicator._item.menu;
    this._bt_level = new bluetooth_battery_level_engine(new GnomeBluetooth.Client());
    this._next_loop_handle = null;
    this._last_devices = [];
    this._refresh_lock = false;
    this._enabled = false;
  }

  async refresh_device(device) {
    if (device.is_connected) {
      let battery_level = await this._bt_level.get_battery_level(device.mac);

      if (Number.isNaN(battery_level)) {
        this._bluetooth_indicator._item.label.text = `${device.name}`;
        if (!this._enabled) { return; }
        return new Promise((resolve) => {
          logger.log('retry (2s)...')
          GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 2, async () => {
            const devices = this._bt_level.get_devices();
            const d = devices.find(d => device.mac === d.mac);
            await this.refresh_device(d);
            resolve();
          });
        });
      }

      this._bluetooth_indicator._item.label.text = `${device.name} (${battery_level} %)`;
    }
  }

  async refresh() {
    if (this._refresh_lock === true) {
      logger.log('Another process is running, refresh canceled');
      return;
    }

    try {
      this._refresh_lock = true;
      logger.log('refreshing');
      const devices = this._bt_level.get_devices();
      const connected_devices_number = this._bt_level.count_connected_devices(devices)
  
      if (connected_devices_number > 1 || connected_devices_number < 1) {
        this._refresh_lock = false;
        return;
      }
      this._last_devices = [];
      await Promise.all(devices.map(async d => {
        if (d.is_connected) {
          this._last_devices.push(d);
          this.refresh_device(d);
        }
      }));
      this._refresh_lock = false;
    } catch (e) {
      this._refresh_lock = false;
      throw (e);
    }
  }

  async loop() {
    try {
      await this.refresh();
    } catch (e) {
      logger.error(e);
    }
    this._next_loop_handle = GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 180, () => {
      this.loop();
    });
  }

  _register_signals() {
    const at = new event_trigger_attenuator(1000);

    this.register_signal(this._bt_level, 'device-inserted', at.wrap((_ctrl, device) => {
      logger.log('got signal "device-inserted" refreshing in 5 seconds');
      GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 5, () => {
        this.refresh();
      });
    }));
    this.register_signal(this._bt_level, 'device-changed', at.wrap((_ctrl, device) => {
      logger.log('got signal "device-changed" refreshing in 5 seconds');
      GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 5, () => {
        this.refresh();
      });
    }));
    this.register_signal(this._bt_level, 'device-deleted', at.wrap(() => {
      logger.log('got signal "device-deleted" refreshing in 5 seconds');
      GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 5, () => {
        this.refresh();
      });
    }));
  }

  _unregister_signals() {
    this.unregister_signal_all();
  }

  enable() {
    this._enabled = true;
    this._register_signals();
    this._bt_level.enable();
    // make sure GnomeBluetooth is ready
    GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 0, () => {
      this.loop()
    });
  }

  disable() {
    this._enabled = false;
    this._bt_level.disable();
    this._unregister_signals();
    if (this._next_loop_handle !== null) {
      GLib.source_remove(this._next_loop_handle);
    }
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;