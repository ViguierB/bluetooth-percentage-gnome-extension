imports.searchPath.push("../gnome-bluetooth-percentage@ben.holidev.net/");

const { bluetooth_battery_level_engine } = imports.srcs.battery_level_engine;


(() => {
  
  let le = new bluetooth_battery_level_engine();
  
  le.test_connection();
    
})();