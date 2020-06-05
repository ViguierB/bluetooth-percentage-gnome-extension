const GLib = imports.gi.GLib;

const Me = imports.misc.extensionUtils.getCurrentExtension();

var { logger } = Me.imports.srcs.misc.logger;
var { signals } = Me.imports.srcs.misc.signals;
var { event_trigger_attenuator } = Me.imports.srcs.misc.event_trigger_attenuator;
var { ptimeout, pinterval } = Me.imports.srcs.misc.promise_timeout;