#include "main.hpp"
#include "config.hpp"
#include "launcher.hpp"
#include <giomm/desktopappinfo.h>

/* Launchers */
launcher::launcher(AppInfo app) : Gtk::Button(), app_info(app) {
	Glib::ustring name = app->get_display_name();
	Glib::ustring description = app->get_description();

	if (items_per_row == 1)
		set_margin_top(app_margin);
	else
		set_margin(app_margin);

	image_program.set(app->get_icon());
	image_program.set_pixel_size(icon_size);

	set_tooltip_text(description);

	if (name.length() > max_name_length)
		name = name.substr(0, max_name_length - 2) + "..";

	label_program.set_text(name);

	int size_request = -1;
	if (name_under_icon) {
		box_launcher.set_orientation(Gtk::Orientation::VERTICAL);
		box_launcher.set_valign(Gtk::Align::CENTER);
		set_valign(Gtk::Align::START);
		size_request = max_name_length * 10;
	}
	else
		label_program.property_margin_start().set_value(10);

	box_launcher.append(image_program);
	box_launcher.append(label_program);

	set_child(box_launcher);

	set_can_focus(false);
	set_hexpand(true);
	set_size_request(size_request, size_request);

	get_style_context()->add_class("flat");
	signal_clicked().connect(sigc::mem_fun(*this, &launcher::on_click));
}

void launcher::on_click() {
	app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
	handle_signal(12);
}

bool launcher::matches(Glib::ustring pattern) {
	Glib::ustring name = app_info->get_name();
	Glib::ustring long_name = app_info->get_display_name();
	Glib::ustring progr = app_info->get_executable();
	Glib::ustring descr = app_info->get_description();

	Glib::ustring text =
		name.lowercase() + "$" +
		long_name.lowercase() + "$" +
		progr.lowercase() + "$" +
		descr.lowercase();

	return text.find(pattern.lowercase()) != text.npos;
}

bool launcher::operator < (const launcher& other) {
	return Glib::ustring(app_info->get_name()).lowercase()
		< Glib::ustring(other.app_info->get_name()).lowercase();
}
