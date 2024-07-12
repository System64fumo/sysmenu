#pragma once
#include "launcher.hpp"
#include "dock.hpp"

#include <gtkmm/window.h>
#include <gtkmm/entry.h>
#include <gtkmm/flowbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/revealer.h>
#include <gtkmm/centerbox.h>
#include <gtkmm/gesturedrag.h>
#include <giomm/desktopappinfo.h>

class sysmenu : public Gtk::Window {
	public:
		sysmenu(const config &cfg);
		void handle_signal(int signum);

	private:
		config config_main;
		int starting_height = 0;
		int max_height;

		int matches = 0;
		Glib::ustring match = "";
		std::vector<std::shared_ptr<Gio::AppInfo>> app_list;
		std::vector<std::unique_ptr<launcher>> items;

		GdkDisplay *display;
		GListModel *monitors;
		GdkMonitor *monitor;
		int monitorCount;

		dock *sysmenu_dock;
		Gtk::Entry entry_search;
		Gtk::Box box_layout;
		Gtk::Revealer revealer_dock;
		Gtk::Revealer revealer_search;
		Gtk::FlowBox flowbox_itembox;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::Box box_top;
		Gtk::CenterBox centerbox_top;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;

		void app_info_changed(GAppInfoMonitor* gappinfomonitor);
		void load_menu_item(const Glib::RefPtr<Gio::AppInfo> &app_info);

		void on_child_activated(Gtk::FlowBoxChild* child);
		bool on_escape_key_press(const guint &keyval, const guint &keycode, const Gdk::ModifierType &state);

		bool on_sort(Gtk::FlowBoxChild*, Gtk::FlowBoxChild*);
		bool on_filter(Gtk::FlowBoxChild* child);
		void on_search_changed();
		void on_search_done();

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);
};

extern "C" {
	sysmenu *sysmenu_create(const config &cfg);
	void syspower_handle_signal(sysmenu *window, int signal);
}
