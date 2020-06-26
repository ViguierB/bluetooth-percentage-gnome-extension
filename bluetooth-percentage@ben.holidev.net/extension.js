/* extension.js
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* exported init */

const Main = imports.ui.main;
const GnomeBluetooth = imports.gi.GnomeBluetooth;

const Me = imports.misc.extensionUtils.getCurrentExtension();
const { bluetooth_battery_level_extention } = Me.imports.srcs.battery_level_extention;
const { logger } = Me.imports.misc;

var instance = null;

function init() {
  const bluetooth_menu = Main.panel.statusArea.aggregateMenu._bluetooth;
  // let settings = Convenience.getSettings();
  
  instance = new bluetooth_battery_level_extention(bluetooth_menu);
}

function enable() {
  const date = new Date();
  logger.open({
    quiet: true,
    // quiet: false,
    log_file: `${Me.path}/.logs/${date.getDate()}-${date.getMonth()}-${date.getFullYear()}`
  });
  instance.enable();
}

function disable() {
  instance.disable();
  logger.close();
}