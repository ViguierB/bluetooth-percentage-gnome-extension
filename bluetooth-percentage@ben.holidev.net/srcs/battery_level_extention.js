const Main = imports.ui.main;
const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const St = imports.gi.St;

const { loadInterfaceXML } = imports.misc.fileUtils;

const BUS_NAME = 'org.gnome.SettingsDaemon.Rfkill';
const OBJECT_PATH = '/org/gnome/SettingsDaemon/Rfkill';

const RfkillManagerInterface = loadInterfaceXML('org.gnome.SettingsDaemon.Rfkill');
const RfkillManagerProxy = Gio.DBusProxy.makeProxyWrapper(RfkillManagerInterface);


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
    this.icons = {
      geticon: (level) => {
        level = level - (level % 10);
        return this.icons._cache[level / 10];
      },
      _cache: []
    };
    for (let i = 1; i <= 10; ++i) {
      this.icons._cache[i] = Gio.icon_new_for_string(`${Me.path}/icons/bluetooth-battery-level-${i}0-symbolic.svg`);
    }
    
    this._proxy = new RfkillManagerProxy(Gio.DBus.session, BUS_NAME, OBJECT_PATH,
      (proxy, error) => {
        if (error) {
          logger.error(error.message);
          return;
        }
      }
    );

    
  }

  show_battery_lvl(device, lvl) {
    let gicon = this.icons.geticon(lvl);
    
    this._bluetooth_indicator._item.label.text = `${device.name} (${lvl} %)`;
    this._bluetooth_indicator._indicator.gicon = gicon;
    this._bluetooth_indicator._item.icon.gicon = gicon;
  }

  _override_bluetooth_menu_sync_member() {
    const initial_sync = this._bluetooth_indicator._sync;

    this._bluetooth_indicator._sync = (function (...args) {
      logger.log("sync called...", args);
      initial_sync.bind(this)(...args);
    }).bind(this._bluetooth_indicator);
  }

  _register_signals() {
    const at = new event_trigger_attenuator(500);
    const refresh = at.wrap(() => {
      const connected_devices = this._bt_level._bt_devices.filter(device => device.is_connected);

      if (connected_devices.length === 1) {
        this.show_battery_lvl(connected_devices[0], connected_devices[0].battery_lvl || 100);
      }
    });

    this.register_signal(this._proxy, 'g-properties-changed', refresh);
    this.register_signal(this._bt_level, [ 'row-changed', 'row-deleted', 'row-insered' ], refresh);
    this.register_signal(Main.sessionMode, 'updated', refresh);
    this.register_signal(this._bt_level, 'battery_level_change', (_source, { device, lvl }) => {
      logger.log('got signal "battery_level_change" from ' + device.name + ': ' + lvl);
      const connected_devices = this._bt_level._bt_devices.filter(device => device.is_connected);

      if (connected_devices.length === 1) {
        this.show_battery_lvl(device, lvl);
      }
    });
  }

  _unregister_signals() {
    this.unregister_signal_all();
  }

  enable() {
    this._enabled = true;
    // this._override_bluetooth_menu_sync_member();
    this._register_signals();
    this._bt_level.enable();
    logger.log('Bluetooth Percentage: Enabled');
  }

  disable() {
    this._enabled = false;
    this._bt_level.disable();
    this._unregister_signals();
    const connected_devices = this._bt_level._bt_devices.filter(device => device.is_connected);
    this._bluetooth_indicator._item.label.text = `${connected_devices.length} connected`;
    logger.log('Bluetooth Percentage: Disabled');
  }
}

var bluetooth_battery_level_extention = _bluetooth_battery_level_extention;