const Main = imports.ui.main;
const GLib = imports.gi.GLib;
const St = imports.gi.St;
const GnomeBluetooth = imports.gi.GnomeBluetooth;

const Me = imports.misc.extensionUtils.getCurrentExtension();
const { bluetooth_battery_level_engine } = Me.imports.srcs.battery_level_engine;
const {
  signals,
  logger,
  event_trigger_attenuator,
  ptimeout
} = Me.imports.misc;

class _bluetooth_battery_level_extention extends signals {
  constructor() {
    super();
    this._bluetooth_indicator = Main.panel.statusArea.aggregateMenu._bluetooth;
    this._menu = this._bluetooth_indicator._item.menu;
    this._bt_level = new bluetooth_battery_level_engine(new GnomeBluetooth.Client());
    this._enabled = false;
  }

  _register_signals() {
    this.register_signal(this._bt_level, 'battery_level_change', (_source, { device, lvl }) => {
      logger.log('got signal "battery_level_change" from ' + device.name + ': ' + lvl);
      this._bluetooth_indicator._item.label.text = `${device.name} (${lvl} %)`;
    });
  }

  _unregister_signals() {
    this.unregister_signal_all();
    const connected_devices = this._bt_level._bt_devices.filter(device => device.is_connected);
    this._bluetooth_indicator._item.label.text = `${connected_devices.length} connected`;
  }

  enable() {
    this._enabled = true;
    this._register_signals();
    this._bt_level.enable();
  }

  disable() {
    this._enabled = false;
    this._bt_level.disable();
    this._unregister_signals();
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;