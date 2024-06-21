#include "dock.hpp"
#include "config.hpp"

dock::dock() {
	get_style_context()->add_class("dock");
	set_halign(Gtk::Align::CENTER);
}

void dock::load_items(std::vector<std::shared_ptr<Gio::AppInfo>> items) {
	for (const auto& app_info : items) {
		std::string name = app_info->get_name();

		if (!app_info->should_show() || !app_info->get_icon())
			continue;

		if (dock_items.find(name) == std::string::npos)
			continue;

		if (dock_existing_items.find(name) != std::string::npos)
			continue;

		dock_existing_items = dock_existing_items + name;
		dock_item item(app_info);
		append(item);
	}
}

dock_item::dock_item(Glib::RefPtr<Gio::AppInfo> app) {
	get_style_context()->add_class("dock_item");
	set_size_request(64, 64);

	// TODO: Add on_click event
	append(image_icon);
	image_icon.set(app->get_icon());
	image_icon.set_pixel_size(64);
}
