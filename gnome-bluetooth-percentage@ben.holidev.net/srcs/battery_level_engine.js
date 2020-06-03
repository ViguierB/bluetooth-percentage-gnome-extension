const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;

const extensionUtils = imports.misc.extensionUtils;
const Me = extensionUtils.getCurrentExtension();


class _bluetooth_battery_level_engine {
  constructor() {}

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

  async get_battery_level(addr) {
    const proc_out = await this._run_battery_level_command(addr);
    if (proc_out.ok) {
      return proc_out.stdout + " %";
    } else {
      error(`battery_level_engine: ${proc_out.stderr}`);
      throw null;
    }
  }

  get_connected_devices() {
    
  }
}

var bluetooth_battery_level_engine = _bluetooth_battery_level_engine;