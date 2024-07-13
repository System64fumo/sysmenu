#include "config.hpp"
#include "launcher.hpp"

launcher::launcher(const config &config_main, Glib::RefPtr<Gio::AppInfo> app) : Gtk::Button(), app_info(app) {
	name = app_info->get_name();
	long_name = app_info->get_display_name();
	progr = app_info->get_executable();
	descr = app_info->get_description();

	if (config_main.items_per_row == 1)
		set_margin_top(config_main.app_margin);
	else
		set_margin(config_main.app_margin);

	image_program.set(app->get_icon());
	image_program.set_pixel_size(config_main.icon_size);

	set_tooltip_text(descr);

	if (long_name.length() > config_main.max_name_length)
		label_program.set_text(long_name.substr(0, config_main.max_name_length - 2) + "..");
	else
		label_program.set_text(long_name);

	int size_request = -1;
	if (config_main.name_under_icon) {
		box_launcher.set_orientation(Gtk::Orientation::VERTICAL);
		box_launcher.set_valign(Gtk::Align::CENTER);
		set_valign(Gtk::Align::START);
		size_request = config_main.max_name_length * 10;
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
}

bool launcher::matches(Glib::ustring pattern) {
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
