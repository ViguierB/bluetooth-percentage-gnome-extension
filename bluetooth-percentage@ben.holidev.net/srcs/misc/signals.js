
var signals = class {

  constructor() {
    this._signals = [];
  }

  register_signal(subject, sig_names, hdl) {
    if (Array.isArray(sig_names) === false) {
      sig_names = [ sig_names ];
    }
    let signal_ids = sig_names.map(sig_name => {
      let signal_id = subject.connect(sig_name, hdl);
      this._signals.push({
        subject: subject,
        signal_id: signal_id
      });
      return signal_id;
    });
    return signal_ids.length === 1 ? signal_ids[0] : signal_ids;
  }

  unregister_signal_all() {
    this._signals.forEach(sig => {
        sig.subject.disconnect(sig.signal_id);
    });
    this._signals = [];
  }

  unregister_signal(signal_id) {
    const idx = this._signals.findIndex(v => v.signal_id === signal_id);

    if (idx !== -1) {
      let [ sig ] = this._signals.splice(idx, 1);
      sig.subject.disconnect(sig.signal_id);
    }
  }

}