#pragma once
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <giomm/desktopappinfo.h>

class launcher : public Gtk::Button {
	public:
		launcher(const config &config_main, Glib::RefPtr<Gio::AppInfo> app);
		Glib::RefPtr<Gio::AppInfo> app_info;

		bool matches(Glib::ustring text);
		bool operator < (const launcher& other);
		void on_click();

	private:
		Gtk::Box box_launcher;
		Gtk::Image image_program;
		Gtk::Label label_program;

		Glib::ustring name;
		Glib::ustring long_name;
		Glib::ustring progr;
		Glib::ustring descr;
};
