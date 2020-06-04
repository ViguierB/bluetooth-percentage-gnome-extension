imports.searchPath.push("../gnome-bluetooth-percentage@ben.holidev.net")

const { bluetooth_battery_level_extention } = imports.srcs.battery_level_extention;

(() => {

  let tmp = new bluetooth_battery_level_extention();

  log(JSON.stringify(tmp.getDevices()));

})();