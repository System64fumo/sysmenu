#pragma once
#include <gtkmm/flowbox.h>
#include <gtkmm/image.h>
#include <giomm/desktopappinfo.h>

class dock_item : public Gtk::FlowBoxChild {
	public:
		dock_item(Glib::RefPtr<Gio::AppInfo> app);
		Glib::RefPtr<Gio::AppInfo> app_info;

	private:
		Gtk::Image image_icon;
};

class dock : public Gtk::FlowBox {
	public:
		dock();
		void load_items(std::vector<std::shared_ptr<Gio::AppInfo>> items);

	private:
		std::string dock_existing_items;
		void on_child_activated(Gtk::FlowBoxChild* child);
};
