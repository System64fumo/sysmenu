#include "launcher.hpp"

launcher::launcher(const std::map<std::string, std::map<std::string, std::string>>& cfg, const Glib::RefPtr<Gio::AppInfo> &app) : Gtk::Box(), app_info(app), config_main(cfg) {
	name = app_info->get_name();
	long_name = app_info->get_display_name();
	progr = app_info->get_executable();
	descr = app_info->get_description();

	if (config_main["main"]["items-per-row"] == "1")
		set_margin_top(std::stoi(config_main["main"]["app-margins"]));
	else
		set_margin(std::stoi(config_main["main"]["app-margins"]));

	image_program.set(app->get_icon());
	image_program.set_pixel_size(std::stoi(config_main["main"]["icon-size"]));

	if (long_name.length() > std::stoul(config_main["main"]["name-length"]))
		label_program.set_text(long_name.substr(0, std::stoi(config_main["main"]["name-length"]) - 2) + "..");
	else
		label_program.set_text(long_name);

	int size_request = -1;
	if (config_main["main"]["name-under-icon"] == "true") {
		set_orientation(Gtk::Orientation::VERTICAL);
		size_request = std::stoi(config_main["main"]["name-length"]) * 10;
		image_program.set_vexpand(true);
		image_program.set_valign(Gtk::Align::END);
		label_program.set_margin_top(3);
		label_program.set_vexpand(true);
		label_program.set_valign(Gtk::Align::START);
	}
	else
		label_program.property_margin_start().set_value(10);

	append(image_program);
	append(label_program);

	set_hexpand(true);
	set_size_request(size_request, size_request);

	get_style_context()->add_class("launcher");
	set_tooltip_text(descr);
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
