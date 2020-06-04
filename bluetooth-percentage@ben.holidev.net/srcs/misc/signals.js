
var signals = class {

  constructor() {
    this._signals = [];
  }

  register_signal(subject, sig_name, hdl) {
    if (!this._signals) this._signals = [];

    let signal_id = subject.connect(sig_name, hdl);
    this._signals.push({
      subject: subject,
      signal_id: signal_id
    });
  }

  unregister_signal_all() {
    if (!this._signals) return;

    this._signals.forEach(sig => {
        sig.subject.disconnect(sig.signal_id);
    });
    this._signals = [];
  }

}