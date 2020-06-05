const GLib = imports.gi.GLib;

var ptimeout = async function(interval = 0, cancel_token = null) {
  cancel_token = cancel_token || ptimeout.create_cancel_token();
  return new Promise((resolve) => {
    const native_token = GLib.timeout_add(GLib.PRIORITY_DEFAULT, interval, () => { cancel_token.cancel(); resolve(); });
    cancel_token.cancel = () => {
      GLib.source_remove(native_token);
    }
  });
}
ptimeout.create_cancel_token = () => ({ cancel: () => {} });

var pinterval = async function(interval = 0, cancel_token = null) {
  return new Promise((resolve) => {
    const native_token = GLib.timeout_add(GLib.PRIORITY_DEFAULT, interval, resolve);
    if (!cancel_token) { return; }
    cancel_token.cancel = () => {
      GLib.source_remove(native_token);
    }
  });
}
pinterval.create_cancel_token = ptimeout.create_cancel_token;