#include "dock.hpp"

#include <iostream>

dock::dock() {
	get_style_context()->add_class("dock");
	set_size_request(-1, 100);
}

void dock::load_items(std::vector<std::shared_ptr<Gio::AppInfo>> items) {
	// TODO: Add check to not re add existing items
	for (const auto& app_info : items) {
		dock_item item(app_info);
		append(item);
	}
}

dock_item::dock_item(Glib::RefPtr<Gio::AppInfo> app) {
	std::cout << app->get_name() << std::endl;
	// TODO: Add the usual stuff (icon, name, on_click action, ect..)
}
