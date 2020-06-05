const GLib = imports.gi.GLib;

imports.searchPath.push("../bluetooth-percentage@ben.holidev.net")
const { logger } = imports.srcs.misc.logger;
let loop = GLib.MainLoop.new(null, false);

(() => {

  logger.open({
    quiet: false,
    log_file: './log'
  });

  
  logger.log('pomme de terre');


  loop.run();
})();