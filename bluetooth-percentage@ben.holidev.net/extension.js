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
  const date = new Date();
  logger.open({
    quiet: true,
    // quiet: false,
    log_file: `${Me.path}/.logs/${date.getDate()}-${date.getMonth()}-${date.getFullYear()}`
  });

  const bluetooth_menu = Main.panel.statusArea.aggregateMenu._bluetooth;
  const theme = imports.gi.Gtk.IconTheme.get_default();
  // let settings = Convenience.getSettings();

  logger.log(Me.path + '/icons');
  theme.append_search_path(Me.path + "/icons");


  instance = new bluetooth_battery_level_extention(bluetooth_menu);
}

// function enable() {
//   instance.enable();
// }

// function disable() {
//   instance.disable();
// }

const PanelMenu = imports.ui.panelMenu;
const Config = imports.misc.config;
const SHELL_MINOR = parseInt(Config.PACKAGE_VERSION.split('.')[1]);
const GObject = imports.gi.GObject;
const St = imports.gi.St;
const Gio = imports.gi.Gio;

var ExampleIndicator = class ExampleIndicator extends PanelMenu.Button {

    _init() {
        super._init(0.0, `${Me.metadata.name} Indicator`, false);

        this.theme_node = this.get_theme_node();
        const colors = this.theme_node.get_icon_colors();

        // Pick an icon
        let icon = new St.Icon({
            style_class: 'system-status-icon'
        });
        icon.gicon = Gio.icon_new_for_string(`${Me.path}/icons/bluetooth-battery-90-symbolic.svg`);
        this.actor.add_child(icon);

        // Add a menu item
        this.menu.addAction('Menu Item', this.menuAction, null);
    }

    menuAction() {
        log('Menu item activated');
    }
}

// Compatibility with gnome-shell >= 3.32
if (SHELL_MINOR > 30) {
    ExampleIndicator = GObject.registerClass(
        {GTypeName: 'ExampleIndicator'},
        ExampleIndicator
    );
}

var indicator = null;

function enable() {
    logger.log(`enabling ${Me.metadata.name} version ${Me.metadata.version}`);

    indicator = new ExampleIndicator();

    // The `main` import is an example of file that is mostly live instances of
    // objects, rather than reusable code. `Main.panel` is the actual panel you
    // see at the top of the screen.
    Main.panel.addToStatusArea(`${Me.metadata.name} Indicator`, indicator);
}


function disable() {
    logger.log(`disabling ${Me.metadata.name} version ${Me.metadata.version}`);

    // REMINDER: It's required for extensions to clean up after themselves when
    // they are disabled. This is required for approval during review!
    if (indicator !== null) {
        indicator.destroy();
        indicator = null;
    }
}