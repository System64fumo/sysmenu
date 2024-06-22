#include "dock.hpp"
#include "config.hpp"

dock::dock() {
	get_style_context()->add_class("dock");
	set_halign(Gtk::Align::CENTER);
	property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_selection_mode(Gtk::SelectionMode::NONE);
	signal_child_activated().connect(sigc::mem_fun(*this, &dock::on_child_activated));
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
		auto item = Gtk::make_managed<dock_item>(app_info);
		append(*item);
	}
}

void dock::on_child_activated(Gtk::FlowBoxChild* child) {
	dock_item *button = dynamic_cast<dock_item*>(child);
	button->app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
}

dock_item::dock_item(Glib::RefPtr<Gio::AppInfo> app) {
	app_info = app;
	get_style_context()->add_class("dock_item");
	set_size_request(64, 64);

	set_child(image_icon);
	image_icon.set(app->get_icon());
	image_icon.set_pixel_size(64);

	set_focus_on_click(false);
}
