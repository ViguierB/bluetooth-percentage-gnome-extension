const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;

class _logger {
  
  constructor() {};

  open(settings) {
    this._ready_queue = [];
    this._is_quiet = settings.quiet;
    this._file = Gio.File.new_for_path(settings.log_file);
    const ready_hdl_factory = (finish_handler) => (source_object, res) => {
      const finish = finish_handler.bind(source_object);

      this._filestream = finish(res);
      if (this._filestream === null || this._filestream.is_closed()) {
        log('CANNOT OPEN LOG FILE');
      }
      this._filestream.seek(0,  GLib.SeekType.END, null);
      this._ready_queue.forEach(resolve => resolve());
      this._ready_queue = [];
    };
    if (this._file.query_exists(null)) {
      this._file.open_readwrite_async(0, null, ready_hdl_factory(this._file.open_readwrite_finish));
    } else {
      this._file.create_readwrite_async(Gio.FileCreateFlags.NONE, 0, null, ready_hdl_factory(this._file.create_readwrite_finish));
    }
    this.log("logger ready");
  }

  async close() {
    return this.ready().then(() => {
      this._filestream.close(null);
      delete this._filestream;
    })
  }

  ready() {
    if (!this._filestream) {
      return new Promise((resolve) => { this._ready_queue.push(resolve); });
    }
    return Promise.resolve();
  }

  _parse_message(message) {
    if (message instanceof Error) {
      return message;
    } else if (typeof message === 'object' || typeof message === 'array') {
      return JSON.stringify(message, null, 2);
    }
    return message;
  }


  _writer_factory(output_channel) {
    return function(message, no_date = false) {
      if (!this._is_quiet) {
        output_channel(message);
      }
      if (message instanceof Error) {
        message = message.toString();
      }
      this.ready().then(() => {
        if (!this._filestream.is_closed()) {
          this._filestream.get_output_stream().write(`${no_date ? '' : (this._get_date() + ' ')}${message}\n`, null);
        }
      });
    }.bind(this);
  }

  _get_date() {
    const date = new Date();

    return `${date.getDate()}/${date.getMonth()}/${date.getFullYear()} ${date.getHours()}:${date.getMinutes()}:${date.getSeconds()}`;
  }

  _write(output_channel, message, title) {
    const w = this._writer_factory(output_channel);
    if (message instanceof Error) {
      w(`[${title}]:`);
      w(message, true);
    } else {
      w(`[${title}]: ${this._parse_message(message)}`);
    }
  }

  log(message) {
    this._write(log, message, '  LOG');
  }

  error(message) {
    this._write(error, message, 'ERROR');
  }

}

var logger = new _logger();