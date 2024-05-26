#pragma once

#include <gtkmm.h>
#include <giomm/desktopappinfo.h>

using AppInfo = Glib::RefPtr<Gio::AppInfo>;

class sysmenu : public Gtk::Window {

	int matches = 0;
	Glib::ustring match = "";

	public:
		Gtk::Entry entry_search;
		Gtk::FlowBox flowbox_itembox;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::Box box_layout;
		Gtk::Box box_grabber;
		Gtk::CenterBox centerbox_top;

		int starting_height = 0;
		int max_height;

		static void app_info_changed(GAppInfoMonitor* gappinfomonitor, gpointer user_data);
		void load_menu_item(AppInfo app_info);
		sysmenu();

	private:
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;

		void InitLayout();
		void on_child_activated(Gtk::FlowBoxChild* child);
		bool on_escape_key_press(guint keyval, guint keycode, Gdk::ModifierType state);

		bool on_sort(Gtk::FlowBoxChild*, Gtk::FlowBoxChild*);
		bool on_filter(Gtk::FlowBoxChild* child);
		void on_search_changed();
		void on_search_done();

		void on_drag_start(int x, int y);
		void on_drag_update(int x, int y);
		void on_drag_stop(int x, int y);
};
