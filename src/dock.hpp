#pragma once
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <giomm/desktopappinfo.h>

class dock_item : public Gtk::Button {
	public:
		dock_item(Glib::RefPtr<Gio::AppInfo> app);
};

class dock : public Gtk::Box {
	public:
		dock();
		void load_items(std::vector<std::shared_ptr<Gio::AppInfo>> items);
};
