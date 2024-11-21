#include "dock.hpp"

dock::dock(const config_menu &cfg) {
	config_main = cfg;
	get_style_context()->add_class("dock");
	set_halign(Gtk::Align::CENTER);
	property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_selection_mode(Gtk::SelectionMode::NONE);
	set_sort_func(sigc::mem_fun(*this, &dock::on_sort));
	signal_child_activated().connect(sigc::mem_fun(*this, &dock::on_child_activated));

	// Make all items lowercase for easier detection
	config_main.dock_items = to_lowercase(config_main.dock_items);

	// Funky sorting
	size_t index = 0;
	std::stringstream ss(config_main.dock_items);
	std::string item;
	while (std::getline(ss, item, ',')) {
		order_map[item] = index++;
	}
}

void dock::load_items(const std::vector<std::shared_ptr<Gio::AppInfo>> &items) {
	dock_existing_items.clear();
	for (const auto& app_info : items) {
		std::string name = to_lowercase(app_info->get_name());

		if (!app_info->should_show() || !app_info->get_icon())
			continue;

		if (config_main.dock_items.find(name) == std::string::npos)
			continue;

		if (dock_existing_items.find(name) != std::string::npos)
			continue;

		dock_existing_items = dock_existing_items + name;
		auto item = Gtk::make_managed<dock_item>(app_info, config_main.dock_icon_size);
		append(*item);
	}
}

void dock::on_child_activated(Gtk::FlowBoxChild* child) {
	dock_item *button = dynamic_cast<dock_item*>(child);
	button->app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
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

bool dock::on_sort(Gtk::FlowBoxChild *a, Gtk::FlowBoxChild *b) {
	// Funky sorting part 2!
	auto appinfo1 = dynamic_cast<dock_item*>(a)->app_info;
	auto appinfo2 = dynamic_cast<dock_item*>(b)->app_info;

	std::string name1 = to_lowercase(appinfo1->get_name());
	std::string name2 = to_lowercase(appinfo2->get_name());

	auto pos1 = order_map.find(name1);
	auto pos2 = order_map.find(name2);

	if (pos1 != order_map.end() && pos2 != order_map.end()) {
		return pos1->second > pos2->second;
	}

	return false;
}

std::string dock::to_lowercase(const std::string &str) {
	std::string result = str;
	for (char& c : result)
		c = std::tolower(c);
	return result;
}
