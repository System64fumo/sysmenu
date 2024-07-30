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
		sysmenu(const config_menu &cfg);
		void handle_signal(const int &signum);

	private:
		config_menu config_main;
		int starting_height = 0;
		int max_height;
		int history_size = 3;

		int matches = 0;
		Glib::ustring match = "";
		std::vector<std::shared_ptr<Gio::AppInfo>> app_list;
		std::vector<std::shared_ptr<Gio::AppInfo>> app_list_history;
		std::vector<std::unique_ptr<launcher>> items;

		GdkDisplay *display;
		GListModel *monitors;
		GdkMonitor *monitor;
		int monitorCount;

		dock *sysmenu_dock;
		Gtk::Entry entry_search;
		Gtk::Box box_layout;
		Gtk::Box box_layout_inner;
		Gtk::Box box_scrolled_contents;
		Gtk::Revealer revealer_dock;
		Gtk::Revealer revealer_search;
		Gtk::FlowBox flowbox_recent;
		Gtk::FlowBox flowbox_itembox;
		Gtk::FlowBoxChild *selected_child;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::ScrolledWindow scrolled_window_inner;
		Gtk::Box box_top;
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;

		void on_search_changed();
		bool on_key_press(const guint &keyval, const guint &keycode, const Gdk::ModifierType &state);

		bool on_filter(Gtk::FlowBoxChild* child);
		bool on_sort(Gtk::FlowBoxChild*, Gtk::FlowBoxChild*);

		void app_info_changed(GAppInfoMonitor* gappinfomonitor);
		void load_menu_item(const Glib::RefPtr<Gio::AppInfo> &app_info);
		void run_menu_item(Gtk::FlowBoxChild* child, const bool &recent);

		void on_drag_start(const double &x, const double &y);
		void on_drag_update(const double &x, const double &y);
		void on_drag_stop(const double &x, const double &y);
};

extern "C" {
	sysmenu *sysmenu_create(const config_menu &cfg);
	void sysmenu_signal(sysmenu *window, int signal);
}
