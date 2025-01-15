#pragma once
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <giomm/desktopappinfo.h>

#include "config.hpp"
#include "dmenuentry.hpp"

class launcher : public Gtk::Box {
	public:
		launcher(const std::map<std::string, std::map<std::string, std::string>>& cfg, const Glib::RefPtr<Gio::AppInfo> &app);
		launcher(const std::map<std::string, std::map<std::string, std::string>>& cfg, const Glib::RefPtr<dmenuentry> &entry);
		Glib::RefPtr<Gio::AppInfo> app_info;

		bool matches(Glib::ustring text);
		bool operator < (const launcher& other);
		Glib::ustring get_long_name();

	private:
		std::map<std::string, std::map<std::string, std::string>> config_main;
		Glib::ustring long_name;
		Gtk::Image image_program;
		Gtk::Label label_program;

		Glib::ustring name;
		Glib::ustring progr;
		Glib::ustring descr;
};
