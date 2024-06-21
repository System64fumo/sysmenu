#pragma once
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <giomm/desktopappinfo.h>

class dock_item : public Gtk::Box {
	public:
		dock_item(Glib::RefPtr<Gio::AppInfo> app);

	private:
		Gtk::Image image_icon;
};

class dock : public Gtk::Box {
	public:
		dock();
		void load_items(std::vector<std::shared_ptr<Gio::AppInfo>> items);
};
