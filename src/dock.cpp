#include "dock.hpp"
#include <gdkmm/display.h>

std::string dock::normalize(const std::string &str) {
	std::string result;
	for (char c : str)
		if (std::isalnum(static_cast<unsigned char>(c)))
			result += std::tolower(static_cast<unsigned char>(c));
	return result;
}

dock::dock(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
	config_main = cfg;
	get_style_context()->add_class("dock");
	set_halign(Gtk::Align::CENTER);
	property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_selection_mode(Gtk::SelectionMode::NONE);
	set_sort_func(sigc::mem_fun(*this, &dock::on_sort));
	signal_child_activated().connect(sigc::mem_fun(*this, &dock::on_child_activated));

	size_t index = 0;
	std::stringstream ss(config_main["main"]["dock-items"]);
	std::string item;
	while (std::getline(ss, item, ','))
		order_map[normalize(item)] = index++;
}

void dock::load_items(const std::vector<std::shared_ptr<Gio::AppInfo>> &items) {
	for (const auto& app_info : items) {
		if (!app_info->should_show() || !app_info->get_icon())
			continue;

		if (get_order_index(app_info) < 0)
			continue;

		auto item = Gtk::make_managed<dock_item>(app_info, std::stoi(config_main["main"]["dock-icon-size"]));
		append(*item);
	}
}

void dock::on_child_activated(Gtk::FlowBoxChild* child) {
	dock_item *button = dynamic_cast<dock_item*>(child);
	if (!button)
		return;

	auto ctx = Gdk::Display::get_default()->get_app_launch_context();
	button->app_info->launch(std::vector<Glib::RefPtr<Gio::File>>{}, ctx);
}

dock_item::dock_item(const Glib::RefPtr<Gio::AppInfo> &app, const int &icon_size) {
	app_info = app;
	get_style_context()->add_class("dock_item");
	set_size_request(icon_size, icon_size);

	set_child(image_icon);
	image_icon.set(app->get_icon());
	image_icon.set_pixel_size(icon_size);

	set_focus_on_click(false);
}

int dock::get_order_index(const Glib::RefPtr<Gio::AppInfo> &app_info) const {
	auto it = order_map.find(normalize(app_info->get_name()));
	if (it != order_map.end())
		return it->second;

	std::string id = app_info->get_id();
	if (id.rfind(".desktop") == id.length() - 8)
		id.resize(id.length() - 8);
	it = order_map.find(normalize(id));
	if (it != order_map.end())
		return it->second;

	return -1;
}

bool dock::on_sort(Gtk::FlowBoxChild *a, Gtk::FlowBoxChild *b) {
	auto item1 = dynamic_cast<dock_item*>(a);
	auto item2 = dynamic_cast<dock_item*>(b);
	if (!item1 || !item2)
		return false;

	int pos1 = get_order_index(item1->app_info);
	int pos2 = get_order_index(item2->app_info);

	if (pos1 >= 0 && pos2 >= 0)
		return pos1 > pos2;

	return false;
}