// vim: tabstop=4 noexpandtab shiftwidth=4 softtabstop=4
/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Alberto Ruiz <alberto.ruiz@canonical.com>
 */

using NM;

namespace Network
{
	private const string APPLICATION_ID  = "com.canonical.indicator.network";
	private const string PHONE_MENU_PATH = "/com/canonical/indicator/network/phone";
	private const string DESKTOP_MENU_PATH = "/com/canonical/indicator/network/desktop";
	private const string ACTION_GROUP_PATH = "/com/canonical/indicator/network";

	internal class ProfileMenu {
		public Menu root_menu;
		public MenuItem root_item;
		public Menu shown_menu;

		private GLib.DBusConnection conn;
		private uint export_id;

		public ProfileMenu (GLib.DBusConnection conn, string path) {
			root_menu = new Menu();
			shown_menu = new Menu();

			root_item = new MenuItem.submenu (null, shown_menu as MenuModel);
			root_item.set_attribute (GLib.Menu.ATTRIBUTE_ACTION, "s", "indicator.global.network-status");
			root_item.set_attribute ("x-canonical-type", "s", "com.canonical.indicator.root");
			root_menu.append_item (root_item);

			export_id = conn.export_menu_model (path, root_menu);
		}

		~ProfileMenu () {
			conn.unexport_menu_model(export_id);
		}
	}

	public class NetworkMenu : GLib.Object
	{
		private ProfileMenu     desktop;
		private NM.Client       client;
		private ActionManager   am;
		private GLibLocal.ActionMuxer muxer = new GLibLocal.ActionMuxer();

		public NetworkMenu ()
		{
			client = new NM.Client();
			am = new ActionManager (muxer, client);

			client.device_added.connect   ((client, device) => { add_device (device); });
			client.device_removed.connect ((client, device) => { remove_device (device); });

			try
			{
				var conn = Bus.get_sync (BusType.SESSION, null);

				conn.export_action_group (ACTION_GROUP_PATH, muxer as ActionGroup);

				desktop = new ProfileMenu(conn, DESKTOP_MENU_PATH);

				Bus.own_name_on_connection(conn, APPLICATION_ID, BusNameOwnerFlags.NONE, null, ((conn, name) => { error("Unable to get D-Bus bus name"); }));
			}
			catch (GLib.IOError e)
			{
				return;
			}
			catch (GLib.Error e)
			{
				return;
			}

			var devices = client.get_devices ();

			if (devices == null)
				return;
			for (uint i = 0; i < devices.length; i++)
			{
				add_device (devices.get (i));
			}
		}

		private Device.Base? device2abstraction (NM.Device device)
		{
			switch (device.get_device_type ())
			{
				case NM.DeviceType.WIFI:
					return new Device.Wifi(this.client, device as NM.DeviceWifi, this.muxer);
				default:
					warning("Unsupported device type: " + device.get_iface());
					break;
			}

			return null;
		}

		private void add_device (NM.Device device)
		{
			Device.Base? founddev = null;

			for (int i = 0; i < (desktop.shown_menu as MenuModel).get_n_items(); i++) {
				var dev = (desktop.shown_menu as MenuModel).get_item_link(i, Menu.LINK_SECTION) as Device.Base;

				if (dev.device.get_path() == device.get_path()) {
					founddev = dev;
					break;
				}
			}

			if (founddev == null) {
				founddev = device2abstraction(device);
				if (founddev != null) {
					/* TODO: We really need to sort these.  For now it's fine. */
					desktop.shown_menu.append_section(null, founddev);
				}

			}
		}

		private void remove_device (NM.Device device)
		{
			for (int i = 0; i < (desktop.shown_menu as MenuModel).get_n_items(); i++) {
				var dev = (desktop.shown_menu as MenuModel).get_item_link(i, Menu.LINK_SECTION) as Device.Base;

				if (dev.device.get_path() == device.get_path()) {
					desktop.shown_menu.remove(i);
					break;
				}
			}
		}
	}
}
