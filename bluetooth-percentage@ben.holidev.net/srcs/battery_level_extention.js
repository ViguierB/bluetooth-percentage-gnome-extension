const Main = imports.ui.main;
const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const St = imports.gi.St;

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
    this._bt_level = new bluetooth_battery_level_engine();
    this._enabled = false;
  }

  _override_bl_menu_label_text_member() {
    this._previous_label_text = this._bluetooth_indicator._item.label.text;
    let   p_text = this._bluetooth_indicator._item.label.text;
    Object.assign(this._bluetooth_indicator._item.label, {
      set text(new_text) {
        logger.log(`A new text was applied: ${new_text}`);
        p_text = new_text;
      },
      get text() {
        logger.log(`get text(): ${p_text}`);
        return p_text;
      }
    });

    logger.log(this._bluetooth_indicator._item.label);
  }

  _register_signals() {
    this.register_signal(this._bt_level, 'battery_level_change', (_source, { device, lvl }) => {
      logger.log('got signal "battery_level_change" from ' + device.name + ': ' + lvl);
      this._bluetooth_indicator._item.label.text = `${device.name} (${lvl} %)`;
      logger.log("icon path = " + `${Me.path}/icons/bluetooth-battery-${lvl}-symbolic.svg`);
      let gicon = Gio.icon_new_for_string(`${Me.path}/icons/bluetooth-battery-${lvl}-symbolic.svg`)
      logger.log(gicon);
      // this._bluetooth_indicator._indicator = this._bluetooth_indicator._addIndicator();
      this._bluetooth_indicator._indicator.gicon = gicon;
      this._bluetooth_indicator._item.icon.gicon = gicon;
      logger.log();
    });
  }

  _unregister_signals() {
    this.unregister_signal_all();
    const connected_devices = this._bt_level._bt_devices.filter(device => device.is_connected);
    this._bluetooth_indicator._item.label.text = `${connected_devices.length} connected`;
  }

  enable() {
    this._enabled = true;
    this._override_bl_menu_label_text_member();
    this._register_signals();
    this._bt_level.enable();
    logger.log('Bluetooth Percentage: Enabled');
  }

  disable() {
    this._enabled = false;
    this._bt_level.disable();
    this._unregister_signals();
    Object.assign(this._bluetooth_indicator._item.label, { text: this._previous_label_text });
    logger.log('Bluetooth Percentage: Disabled');
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;