const GLib = imports.gi.GLib;

class _null_handle {}
var event_trigger_attenuator = class {
  constructor(wait_time) {
    this._wait_time = wait_time || 1000;
    this._null_handle = new _null_handle();
    this._handle = this._null_handle;
  }

  wrap(handler) {
    return () => {
      if (!(this._handle instanceof _null_handle)) {
        GLib.source_remove(this._handle);
      }
      this._handle = GLib.timeout_add(GLib.PRIORITY_DEFAULT, this._wait_time, () => { this._handle = this._null_handle; handler(); });
    }
  }
};
