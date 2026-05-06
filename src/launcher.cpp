#include "launcher.hpp"
#include <glibmm/markup.h>

launcher::launcher(const std::map<std::string, std::map<std::string, std::string>>& cfg, const Glib::RefPtr<Gio::AppInfo> &app, bool is_new) : Gtk::Box(), app_info(app), config_main(cfg) {
	name = app_info->get_name();
	long_name = app_info->get_display_name();
	progr = app_info->get_executable();
	descr = app_info->get_description();
	name_length = std::stoi(config_main["main"]["name-length"]);

	if (config_main["main"]["items-per-row"] == "1")
		set_margin_top(std::stoi(config_main["main"]["app-margins"]));
	else
		set_margin(std::stoi(config_main["main"]["app-margins"]));

	image_program.set(app->get_icon());
	image_program.set_pixel_size(std::stoi(config_main["main"]["icon-size"]));

	Glib::ustring display_name = get_display_name();

	if (is_new)
		label_program.set_markup("<span foreground='#3584e4'>● </span>" + Glib::Markup::escape_text(display_name));
	else
		label_program.set_text(display_name);

	int size_request = -1;
	if (config_main["main"]["name-under-icon"] == "true") {
		set_orientation(Gtk::Orientation::VERTICAL);
		size_request = name_length * 10;
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

	add_css_class("launcher");
	set_tooltip_text(descr);
}

Glib::ustring launcher::get_display_name() const {
	if (long_name.length() > static_cast<size_t>(name_length))
		return long_name.substr(0, name_length - 2) + "..";
	return long_name;
}

bool launcher::matches(Glib::ustring pattern) {
	if (pattern.empty())
		return true;
	auto lower = pattern.lowercase();
	return name.lowercase().find(lower) != name.npos
		|| long_name.lowercase().find(lower) != long_name.npos
		|| progr.lowercase().find(lower) != progr.npos
		|| descr.lowercase().find(lower) != descr.npos;
}

bool launcher::operator < (const launcher& other) {
	return name.lowercase() < other.name.lowercase();
}

void launcher::clear_new_indicator() {
	label_program.set_text(get_display_name());
}
