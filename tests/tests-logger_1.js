const GLib = imports.gi.GLib;

imports.searchPath.push("../bluetooth-percentage@ben.holidev.net")
const { logger } = imports.srcs.misc.logger;
const { ptimeout } = imports.srcs.misc.promise_timeout;

let loop = GLib.MainLoop.new(null, false);

ptimeout().then(async () => {

  logger.open({
    quiet: false,
    log_file: './log_test'
  });

  
  logger.log('pomme de terre');

  await logger.close();

  logger.open({
    quiet: true,
    log_file: './log_test'
  });

  
  logger.log('pomme de terre quiet');


});


loop.run();