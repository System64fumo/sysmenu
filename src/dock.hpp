#pragma once
#include "config.hpp"

#include <gtkmm/flowbox.h>
#include <gtkmm/image.h>
#include <giomm/desktopappinfo.h>

class dock_item : public Gtk::FlowBoxChild {
	public:
		dock_item(const Glib::RefPtr<Gio::AppInfo> &app, const int &icon_size);
		Glib::RefPtr<Gio::AppInfo> app_info;

	private:
		Gtk::Image image_icon;
};

class dock : public Gtk::FlowBox {
	public:
		dock(const std::map<std::string, std::map<std::string, std::string>>& cfg);
		void load_items(const std::vector<std::shared_ptr<Gio::AppInfo>> &items);

	private:
		std::map<std::string, std::map<std::string, std::string>> config_main;
		std::map<std::string, int> order_map;
		std::string dock_existing_items;
		void on_child_activated(Gtk::FlowBoxChild* child);
		bool on_sort(Gtk::FlowBoxChild *a, Gtk::FlowBoxChild *b);
		std::string to_lowercase(const std::string &str);
};
