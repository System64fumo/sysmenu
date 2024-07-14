#pragma once
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <giomm/desktopappinfo.h>

#include "config.hpp"

class launcher : public Gtk::Box {
	public:
		launcher(const config_menu &config_main, const Glib::RefPtr<Gio::AppInfo> &app);
		Glib::RefPtr<Gio::AppInfo> app_info;

		bool matches(Glib::ustring text);
		bool operator < (const launcher& other);

	private:
		Gtk::Image image_program;
		Gtk::Label label_program;

		Glib::ustring name;
		Glib::ustring long_name;
		Glib::ustring progr;
		Glib::ustring descr;
};
