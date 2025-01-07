#include "main.hpp"
#include "window.hpp"
#include "css.hpp"
#include "config.hpp"

#include <gtkmm/eventcontrollerkey.h>
#include <gtk4-layer-shell.h>
#include <thread>
#include <iostream>
#include <filesystem>
#include <glibmm/main.h>
#include <algorithm>

sysmenu::sysmenu(const std::map<std::string, std::map<std::string, std::string>>& cfg) : config_main(cfg) {

	if (config_main["main"]["layer-shell"] == "true") {
		gtk_layer_init_for_window(gobj());
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
		gtk_layer_set_namespace(gobj(), "sysmenu");

		bool edge_top = (config_main["main"]["anchors"].find("top") != std::string::npos);
		bool edge_right = (config_main["main"]["anchors"].find("right") != std::string::npos);
		bool edge_bottom = (config_main["main"]["anchors"].find("bottom") != std::string::npos);
		bool edge_left = (config_main["main"]["anchors"].find("left") != std::string::npos);

		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, edge_top);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, edge_right);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, edge_bottom);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, edge_left);

		if (config_main["main"]["dock-items"] != "")
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
	}

	// Dock
	if (config_main["main"]["dock-items"] != "") {
		sysmenu_dock = Gtk::make_managed<dock>(config_main);
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
		// TODO: Dragging causes the inner scrollbox to resize, This is bad as
		// it uses a lot of cpu power trying to resize things.
		// Is this even possible to fix?

		// Set up gestures
		gesture_drag = Gtk::GestureDrag::create();
		gesture_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &sysmenu::on_drag_start));
		gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysmenu::on_drag_update));
		gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysmenu::on_drag_stop));

		// Set up revealer
		revealer_dock.set_child(*sysmenu_dock);
		revealer_dock.set_transition_type(Gtk::RevealerTransitionType::SLIDE_UP);
		revealer_dock.set_transition_duration(500);
		revealer_dock.set_reveal_child(true);
		revealer_search.set_reveal_child(false);

		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
		box_layout.append(box_top);
		box_top.append(revealer_dock);
		box_top.property_orientation().set_value(Gtk::Orientation::VERTICAL);
		box_top.add_controller(gesture_drag);

		// Set window height to the dock's height
		config_main["main"]["height"] = "30";
		box_layout.set_valign(Gtk::Align::END);
	}
	else
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_TOP);

	// Initialize
	set_name("sysmenu");
	set_default_size(std::stoi(config_main["main"]["width"]), std::stoi(config_main["main"]["height"]));
	set_hide_on_close(true);
	if (config_main["main"]["start-hidden"] != "true") {
		show();
		Glib::signal_timeout().connect_once([&]() {
			get_style_context()->add_class("visible");
		}, 100);
	}

	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_child(box_layout);
	box_layout.append(scrolled_window_inner);
	scrolled_window_inner.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);
	scrolled_window_inner.set_child(box_layout_inner);
	scrolled_window_inner.set_kinetic_scrolling(false);
	box_layout_inner.set_orientation(Gtk::Orientation::VERTICAL);
	box_layout_inner.get_style_context()->add_class("box_layout_inner");

	// Sadly there does not seem to be a way to detect what the default monitor is
	// Gotta assume or ask the user for their monitor of choice
	display = gdk_display_get_default();
	monitors = gdk_display_get_monitors(display);
	monitorCount = g_list_model_get_n_items(monitors);
	monitor = GDK_MONITOR(g_list_model_get_item(monitors, std::stoi(config_main["main"]["monitor"])));

	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);
	max_height = geometry.height;

	// Keep the values in check
	if (std::stoi(config_main["main"]["monitor"]) < 0)
		config_main["main"]["monitor"] = "0";
	else if (std::stoi(config_main["main"]["monitor"]) >= monitorCount)
		config_main["main"]["monitor"] = std::to_string(monitorCount - 1);
	else if (config_main["main"]["layer-shell"] == "true")
		gtk_layer_set_monitor(gobj(), GDK_MONITOR(g_list_model_get_item(monitors, std::stoi(config_main["main"]["monitor"]))));

	// Events
	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(
		sigc::mem_fun(*this, &sysmenu::on_key_press), true);

	if (config_main["main"]["searchbar"] == "true") {
		entry_search.add_controller(controller);
		entry_search.get_style_context()->add_class("entry_search");
		if (config_main["main"]["dock-items"] != "") {
			box_top.append(revealer_search);
			revealer_search.get_style_context()->add_class("revealer_search");
			revealer_search.set_child(entry_search);
			revealer_search.set_transition_type(Gtk::RevealerTransitionType::SLIDE_UP);
			revealer_search.set_transition_duration(500);
		}
		else {
			box_layout.prepend(entry_search);
		}

		entry_search.set_halign(Gtk::Align::CENTER);
		entry_search.set_icon_from_icon_name("system-search-symbolic", Gtk::Entry::IconPosition::PRIMARY);
		entry_search.set_placeholder_text("Search");
		entry_search.set_margin(10);
		entry_search.set_size_request(std::stoi(config_main["main"]["width"]) - 20, -1);

		entry_search.signal_changed().connect(sigc::mem_fun(*this, &sysmenu::on_search_changed));
		entry_search.signal_activate().connect([this]() {
			if (selected_child)
				run_menu_item(selected_child, false);
		});
		flowbox_itembox.set_sort_func(sigc::mem_fun(*this, &sysmenu::on_sort));
		flowbox_itembox.set_filter_func(sigc::mem_fun(*this, &sysmenu::on_filter));

		if (config_main["main"]["dock-items"] == "")
			entry_search.grab_focus();
	}
	else
		add_controller(controller);

	box_layout.get_style_context()->add_class("box_layout");

	if (std::stoi(config_main["main"]["items-per-row"]) != 1) {
		flowbox_itembox.set_halign(Gtk::Align::CENTER);
		history_size = std::stoi(config_main["main"]["items-per-row"]);
	}

	// Recently launched
	// TODO: Add history size config option
	if (history_size > 0) {
		if (std::stoi(config_main["main"]["items-per-row"]) != 1) {
			box_layout_inner.append(flowbox_recent);
			flowbox_recent.set_halign(Gtk::Align::CENTER);
			flowbox_recent.set_orientation(Gtk::Orientation::HORIZONTAL);
		}
		else
			box_scrolled_contents.append(flowbox_recent);

		flowbox_recent.set_visible(false);
		flowbox_recent.get_style_context()->add_class("flowbox_recent");
		flowbox_recent.set_valign(Gtk::Align::START);
		flowbox_recent.set_vexpand_set(true);
		flowbox_recent.set_min_children_per_line(std::stoi(config_main["main"]["items-per-row"]));
		flowbox_recent.set_max_children_per_line(std::stoi(config_main["main"]["items-per-row"]));
		flowbox_recent.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
			run_menu_item(child, true);
		});
	}

	box_layout_inner.append(scrolled_window);
	scrolled_window.set_child(box_scrolled_contents);
	box_scrolled_contents.set_orientation(Gtk::Orientation::VERTICAL);
	box_scrolled_contents.append(flowbox_itembox);

	if (config_main["main"]["searchbar"] != "true")
		scrolled_window.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);

	flowbox_itembox.set_valign(Gtk::Align::START);
	flowbox_itembox.set_min_children_per_line(std::stoi(config_main["main"]["items-per-row"]));
	flowbox_itembox.set_max_children_per_line(std::stoi(config_main["main"]["items-per-row"]));
	flowbox_itembox.set_vexpand(true);
	flowbox_itembox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		run_menu_item(child, false);
	});

	// Load custom css
	std::string style_path;
	if (std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/menu/style.css"))
		style_path = std::string(getenv("HOME")) + "/.config/sys64/menu/style.css";
	else if (std::filesystem::exists("/usr/share/sys64/menu/style.css"))
		style_path = "/usr/share/sys64/menu/style.css";
	else
		style_path = "/usr/local/share/sys64/menu/style.css";

	css_loader loader(style_path, this);

	// Load applications
	GAppInfoMonitor* app_info_monitor = g_app_info_monitor_get();
	g_signal_connect(app_info_monitor, "changed", G_CALLBACK(+[](GAppInfoMonitor* monitor, gpointer user_data) {
		sysmenu* self = static_cast<sysmenu*>(user_data);
		self->app_info_changed(monitor);
	}), this);

	std::thread thread_appinfo(&sysmenu::app_info_changed, this, nullptr);
	thread_appinfo.detach();
}

void sysmenu::on_search_changed() {
	flowbox_recent.set_visible(entry_search.get_text() == "" && app_list_history.size() > 0);
	matches = 0;
	match = "";
	selected_child = nullptr;
	flowbox_itembox.invalidate_filter();
}

bool sysmenu::on_key_press(const guint &keyval, const guint &keycode, const Gdk::ModifierType &state) {
	if (keyval == 65307) // Escape key
		handle_signal(12);
	else if (keyval == 65289) { // Tab
		auto children = flowbox_itembox.get_children();

		if (selected_child == nullptr)
			selected_child = dynamic_cast<Gtk::FlowBoxChild*>(children[0]);

		flowbox_itembox.select_child(*selected_child);
		selected_child->grab_focus();
	}

	return true;
}

bool sysmenu::on_filter(Gtk::FlowBoxChild *child) {
	auto button = dynamic_cast<launcher*> (child->get_child());
	auto text = entry_search.get_text();

	if (button->matches(text)) {
		matches++;
		if (matches == 1) {
			selected_child = child;
			match = button->app_info->get_executable();
		}
		return true;
	}

	return false;
}

bool sysmenu::on_sort(Gtk::FlowBoxChild* a, Gtk::FlowBoxChild* b) {
	auto b1 = dynamic_cast<launcher*> (a->get_child());
	auto b2 = dynamic_cast<launcher*> (b->get_child());
	return *b2 < *b1;
}

void sysmenu::app_info_changed(GAppInfoMonitor* gappinfomonitor) {
	app_list = Gio::AppInfo::get_all();
	flowbox_itembox.remove_all();

	if (config_main["main"]["dock-items"] != "")
		sysmenu_dock->remove_all();

	// Load applications
	for (auto app : app_list)
		load_menu_item(app);

	// Load dock items
	if (config_main["main"]["dock-items"] != "")
		sysmenu_dock->load_items(app_list);

	selected_child = nullptr;
}

void sysmenu::load_menu_item(const Glib::RefPtr<Gio::AppInfo> &app_info) {
	if (!app_info || !app_info->should_show() || !app_info->get_icon())
		return;

	auto name = app_info->get_name();
	auto exec = app_info->get_executable();

	// Skip loading empty entries
	if (name.empty() || exec.empty())
		return;

	items.push_back(std::unique_ptr<launcher>(new launcher(config_main, app_info)));
	flowbox_itembox.append(*items.back());
}

void sysmenu::run_menu_item(Gtk::FlowBoxChild* child, const bool &recent) {
	launcher *item = dynamic_cast<launcher*>(child->get_child());
	item->app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
	handle_signal(12);

	// Don't add the item again if the click came from the recent's list
	if (recent)
		return;

	auto it = std::find(app_list_history.begin(), app_list_history.end(), item->app_info);
	if (it != app_list_history.end())
		return;

	if (app_list_history.size() >= history_size) {
		auto first_child = flowbox_recent.get_child_at_index(0);
		flowbox_recent.remove(*first_child);
		app_list_history.erase(app_list_history.begin());
	}

	launcher *recent_item = new launcher(config_main, item->app_info);
	recent_item->set_size_request(-1, -1);
	flowbox_recent.append(*recent_item);
	app_list_history.push_back(item->app_info);
	flowbox_recent.set_visible(true);
}

void sysmenu::handle_signal(const int &signum) {
	Glib::signal_idle().connect([this, signum]() {
		switch (signum) {
			case 10: // Showing window
				get_style_context()->add_class("visible");
				gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_TOP);
				flowbox_itembox.unselect_all();
				if (config_main["main"]["dock-items"] != "") {
					revealer_search.set_reveal_child(true);
					revealer_dock.set_reveal_child(false);
					box_layout.set_valign(Gtk::Align::FILL);
					box_layout.set_size_request(-1, max_height);
					gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
				}
				show();
				if (config_main["main"]["searchbar"] == "true" && config_main["main"]["dock-items"] == "")
					entry_search.grab_focus();

				break;
			case 12: // Hiding window
				get_style_context()->remove_class("visible");
				if (config_main["main"]["dock-items"] != "") {
					revealer_search.set_reveal_child(false);
					revealer_dock.set_reveal_child(true);
					box_layout.set_valign(Gtk::Align::END);
					box_layout.set_size_request(-1, -1);
					gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
					gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
				}
				else {
					Glib::signal_timeout().connect_once([&]() {
						hide();
						gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
					}, std::stoi(config_main["main"]["animation-duration"]));
				}
				if (config_main["main"]["searchbar"] == "true")
					entry_search.set_text("");
				flowbox_recent.unselect_all();
				flowbox_itembox.unselect_all();

				break;
			case 34: // Toggling window
				if (config_main["main"]["dock-items"] != "") {
					starting_height = box_layout.get_height();
					if (box_layout.get_height() < max_height / 2)
						handle_signal(10);
					else
						handle_signal(12);
				}
				else {
					if (is_visible())
						handle_signal(12);
					else
						handle_signal(10);
				}
				break;
		}
		return false;
	});
}

void sysmenu::on_drag_start(const double &x, const double &y) {
	// For now disable swipe gestures on non touch inputs
	// since they're broken on on touch devices
	if (!gesture_drag->get_current_event()->get_pointer_emulated()) {
		gesture_drag->reset();
		return;
	}

	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);
	max_height = geometry.height;

	starting_height = box_layout.get_height();
	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
	box_layout.set_valign(Gtk::Align::END);
}

void sysmenu::on_drag_update(const double &x, const double &y) {
	bool margin_ignore = (-y < std::stoi(config_main["main"]["dock-icon-size"]) / 2);
	int height = starting_height - y;

	// This mess right here clamps the height to 0 & max possible height
	// It also adds a margin where you have to pull at least 50% of the dock's..
	// height to trigger a pull.
	if (starting_height < max_height) {
		// Pulled from bottom
		if (y > 0 || height >= max_height)
			return;
		else if (margin_ignore)
			height = 0;
	}
	else {
		// Pulled from top
		if (y < 0 || height < std::stoi(config_main["main"]["dock-icon-size"]))
			return;
	}

	box_layout.set_size_request(-1, height);

	gtk_layer_set_layer(gobj(), margin_ignore ? GTK_LAYER_SHELL_LAYER_TOP : GTK_LAYER_SHELL_LAYER_BOTTOM);
	revealer_dock.set_reveal_child(margin_ignore);
	revealer_search.set_reveal_child(!margin_ignore);
}

void sysmenu::on_drag_stop(const double &x, const double &y) {
	// For now disable swipe gestures on non touch inputs
	// since they're broken on on touch devices
	if (!gesture_drag->get_current_event()->get_pointer_emulated()) {
		gesture_drag->reset();
		return;
	}

	// Top position
	if (box_layout.get_height() > max_height / 2)
		handle_signal(10);
	// Bottom Position
	else
		handle_signal(12);
}

extern "C" {
	sysmenu *sysmenu_create(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
		return new sysmenu(cfg);
	}
	void sysmenu_signal(sysmenu *window, int signal) {
		window->handle_signal(signal);
	}
}
