const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const Signals = imports.signals;
const GnomeBluetooth = imports.gi.GnomeBluetooth;
const ByteArray = imports.byteArray;

const extensionUtils = imports.misc.extensionUtils;
const Me = extensionUtils.getCurrentExtension();

const {
  signals,
  logger,
  ptimeout
} = Me.imports.misc;

class _bluetooth_battery_level_engine extends signals {
  constructor() {
    super();
  }

  enable() {
    this._bt_client = new GnomeBluetooth.Client();
    this._bt_model = this._bt_client.get_model();
    this._bt_devices = [];

    this.register_signal(this._bt_model, 'row-changed', (_1, _2, iter) => {
      // logger.log('got signal "row-changed" from the bluetooth model');
      if (iter) {
        let device = new bluetooth_device(this._bt_model, iter);
        this.update_device(device);
      }
    });
    this.register_signal(this._bt_model, 'row-deleted', (_1, _2, iter) => {
      // logger.log('got signal "row-deleted" from the bluetooth model');
      if (iter) {
        let device = new bluetooth_device(this._bt_model, iter);
        this.remove_device(device);
      }
    });
    this.register_signal(this._bt_model, 'row-inserted', (_1, _2, iter) => {
      // logger.log('got signal "row-inserted" from the bluetooth model');
      if (iter) {
        let device = new bluetooth_device(this._bt_model, iter);
        this.add_device(device);
      }
    });
  }

  disable() {
    this.unregister_signal_all();
    this._bt_devices.forEach(d => d.stop());
  }

  remove_device(device) {
    let device_index = this._bt_devices.findIndex(d => d.mac === device.mac);
    if (device_index >= 0) {
      let [ removed ] = this._bt_devices.splice(device_index, 1);
      removed.delete();
    }
  }

  update_device(device) {
    let real_device = this._bt_devices.find(bt_device => bt_device.mac === device.mac);
    if (!!real_device) {
      real_device.merge(device);
      real_device.start();
    } else {
      logger.error(new Error(`${device.name} (${device.mac}) have nothing to do here!`));
    }
  }

  add_device(device) {
    if (!this._bt_devices.find(bt_device => bt_device.mac === device.mac)) {
      this._bt_devices.push(device);
      device.set_battery_change_handler(lvl => {
        this.emit('battery_level_change', {
          device, lvl
        })
      });
      device.start();
    }
  }

  count_connected_devices(devices) {
    let n = 0;

    devices.forEach(d => { n += d.is_connected ? 1 : 0; });
    return n;
  }
}

Signals.addSignalMethods(_bluetooth_battery_level_engine.prototype);

class bluetooth_device {

  constructor(model, ll_device) {
    this.name = model.get_value(ll_device, GnomeBluetooth.Column.NAME);
    this.is_connected = model.get_value(ll_device, GnomeBluetooth.Column.CONNECTED);
    this.is_paired = model.get_value(ll_device, GnomeBluetooth.Column.PAIRED);
    this.mac = model.get_value(ll_device, GnomeBluetooth.Column.ADDRESS);
    this.is_default = model.get_value(ll_device, GnomeBluetooth.Column.DEFAULT);
    this.battery_lvl = null;

    this._is_started = false;
    this.__battery_lvl_change_handler = { success: (_lvl) => {}, error: (message) => {
      logger.error(`out_reader of ${this.name} (${this.mac}) has got an error: ${message}`);
    }};
  }

  merge(device) {
    this.name = device.name; 
    this.is_connected = device.is_connected; 
    this.is_paired = device.is_paired; 
    this.mac = device.mac; 
    this.is_default = device.is_default; 
  }

  _start_engine_process() {
    let [
      success, pid, in_fd, out_fd, err_fd
    ]  = GLib.spawn_async_with_pipes(`${Me.path}/bin`, ['./battery_level_engine', this.mac], null, 0, null);
    
    
    if (success) {
      logger.log('engine process started: pid = ' + pid);
      this._pid = pid;

      this._in_writer = new Gio.DataOutputStream({
        base_stream: new Gio.UnixOutputStream({ fd: in_fd })
      });

      this._out_reader = new Gio.DataInputStream({
        base_stream: new Gio.UnixInputStream({ fd: out_fd })
      });
      this._out_reader.read_line_async(GLib.PRIORITY_DEFAULT, null, (source_object, res) => {
        let [out, _size] = source_object.read_line_finish(res);
        if (!!out) {
          let lvl = parseInt(ByteArray.toString(out), 10);
          if (!Number.isNaN(lvl)) {
            this.battery_lvl = lvl;
            this.__battery_lvl_change_handler.success(lvl);
          } else {
            this.__battery_lvl_change_handler.error(ByteArray.toString(out));
          }
        }
      });

      this._err_reader = new Gio.DataInputStream({
        base_stream: new Gio.UnixInputStream({ fd: err_fd })
      });
      this._err_reader.read_line_async(GLib.PRIORITY_DEFAULT, null, (source_object, res) => {
        let [out, _size] = source_object.read_line_finish(res);
        if (!!out) {
          logger.error(this.name + ': std::err: ' + ByteArray.toString(out));
        }
      });
    } else {
      logger.error(new Error('cannot start the engine process :/'));
    }
    return success;
  }

  set_battery_change_handler(success, error) {
    this.__battery_lvl_change_handler = { 
      success: success || this.__battery_lvl_change_handler.success,
      error: error || this.__battery_lvl_change_handler.error
    };
  }

  start() {
    if (this.is_connected === true && this._is_started === false) {
      logger.log(`device ${this.name} is connected, start process...`);
      try {
        this._start_engine_process();
        this._is_started = true;
      } catch (e) {
        logger.error(e);
      }
    }
  }

  stop() {
    if (this._is_started) {
      this._in_writer.write('stop', null);
      delete this._in_writer;
      delete this._out_reader;
      delete this._err_reader;

      this._is_started = false;
    }
  }

  delete() {
    logger.log("idk what to do for now... :'(");
    stop();
  }

}

var bluetooth_battery_level_engine = _bluetooth_battery_level_engine;