const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const Signals = imports.signals;
const GnomeBluetooth = imports.gi.GnomeBluetooth;

const extensionUtils = imports.misc.extensionUtils;
const Me = extensionUtils.getCurrentExtension();

const { signals } = Me.imports.srcs.misc.signals;


class _bluetooth_battery_level_engine extends signals {
  constructor(gnome_bluetooth_client) {
    super();
    this._bt_client = gnome_bluetooth_client;
    this._bt_model = this._bt_client.get_model();
  }

  enable() {
    this.register_signal(this._bt_model, 'row-changed', (_1, _2, iter) => {
        if (iter) {
            let device = new bluetooth_device(this._bt_model, iter);
            this.emit('device-changed', device);
        }

    });
    this.register_signal(this._model, 'row-deleted', () => {
        this.emit('device-deleted');
    });
    this.register_signal(this._model, 'row-inserted', (_1, _2, iter) => {
        if (iter) {
            let device = new bluetooth_device(this._bt_model, iter);
            this.emit('device-inserted', device);
        }
    });
  }

  disable() {
    this.unregister_signal_all();
  }

  async _run_battery_level_command(addr, channel = 10, cancellable = null) {
    const flags = Gio.SubprocessFlags.STDOUT_PIPE | Gio.SubprocessFlags.STDERR_PIPE;
    const proc = new Gio.Subprocess({
      argv: [`${Me.path}/bin/battery_level_engine`, addr, channel.toString()],
      flags: flags
    });

    proc.init(cancellable);
    return new Promise((resolve, reject) => {
      proc.communicate_utf8_async(null, cancellable, (proc, res) => {
        try {
          let [ok, stdout, stderr] = proc.communicate_utf8_finish(res);
          resolve({
            ok,
            stdout,
            stderr
          });
        } catch (e) {
          reject(e);
        }
      });
    });
  }

  // from https://github.com/bjarosze/gnome-bluetooth-quick-connect/blob/master/bluetooth.js
  get_devices() {
    let adapter = this._get_default_adapter();
    if (!adapter)
      return [];
    let devices = [];

    let [ret, iter] = this._bt_model.iter_children(adapter);
    while (ret) {
      devices.push(new bluetooth_device(this._bt_model, iter));
      ret = this._bt_model.iter_next(iter);
    }

    return devices;
  }

  // from https://github.com/bjarosze/gnome-bluetooth-quick-connect/blob/master/bluetooth.js
  _get_default_adapter() {
    let [ret, iter] = this._bt_model.get_iter_first();
    while (ret) {
      let is_default = this._bt_model.get_value(iter, GnomeBluetooth.Column.DEFAULT);
      let is_powered = this._bt_model.get_value(iter, GnomeBluetooth.Column.POWERED);
      if (is_default && is_powered)
        return iter;
      ret = this._bt_model.iter_next(iter);
    }
    return null;
  }

  async get_battery_level(addr) {
    const proc_out = await this._run_battery_level_command(addr);
    if (proc_out.ok) {
      return Number.parseInt(proc_out.stdout);
    } else {
      error(`battery_level_engine: ${proc_out.stderr}`);
      throw null;
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
  }

}

var bluetooth_battery_level_engine = _bluetooth_battery_level_engine;