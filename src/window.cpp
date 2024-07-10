#include "main.hpp"
#include "window.hpp"
#include "css.hpp"
#include "config.hpp"

#include <gtkmm/eventcontrollerkey.h>
#include <giomm/desktopappinfo.h>
#include <gtk4-layer-shell.h>
#include <thread>
#include <iostream>

void sysmenu::app_info_changed(GAppInfoMonitor* gappinfomonitor) {
	app_list = Gio::AppInfo::get_all();
	flowbox_itembox.remove_all();

	// Load applications
	for (auto app : app_list)
		load_menu_item(app);

	// Load dock items
	if (config_main.dock_items != "")
		sysmenu_dock->load_items(app_list);
}

sysmenu::sysmenu(const config &cfg) {
	config_main = cfg;

	if (config_main.layer_shell) {
		gtk_layer_init_for_window(gobj());
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
		gtk_layer_set_namespace(gobj(), "sysmenu");

		if (config_main.fill_screen) {
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
			if (config_main.dock_items == "")
				gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
		}
	}

	// Dock
	if (config_main.dock_items != "") {
		sysmenu_dock = Gtk::make_managed<dock>(config_main);
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
		// TODO: Gestures currently have issues on non touchscreen inputs,
		// Ideally this should be fixed..
		// However we don't live in an ideal world so tough luck!

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
		//box_top.set_size_request(-1, 100);

		// Set window height to the dock's height
		config_main.height = 30;
		box_layout.set_valign(Gtk::Align::END);
	}
	else
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);

	// Initialize
	set_default_size(config_main.width, config_main.height);
	set_hide_on_close(true);
	if (!config_main.starthidden)
		show();
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_child(box_layout);

	// Sadly there does not seem to be a way to detect what the default monitor is
	// Gotta assume or ask the user for their monitor of choice
	display = gdk_display_get_default();
	monitors = gdk_display_get_monitors(display);
	monitorCount = g_list_model_get_n_items(monitors);
	monitor = GDK_MONITOR(g_list_model_get_item(monitors, config_main.main_monitor));

	// Keep the values in check
	if (config_main.main_monitor < 0)
		config_main.main_monitor = 0;
	else if (config_main.main_monitor >= monitorCount)
		config_main.main_monitor = monitorCount - 1;
	else if (config_main.layer_shell)
		gtk_layer_set_monitor(gobj(), GDK_MONITOR(g_list_model_get_item(monitors, config_main.main_monitor)));

	// Events 
	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(
	sigc::mem_fun(*this, &sysmenu::on_escape_key_press), true);

	if (config_main.searchbar) {
		entry_search.add_controller(controller);
		entry_search.get_style_context()->add_class("searchbar");
		if (config_main.dock_items != "") {
			box_top.append(revealer_search);
			revealer_search.set_child(centerbox_top);
			revealer_search.set_transition_type(Gtk::RevealerTransitionType::SLIDE_UP);
			revealer_search.set_transition_duration(500);
		}
		else {
			box_layout.append(centerbox_top);
		}

		centerbox_top.get_style_context()->add_class("centerbox_top");
		centerbox_top.set_center_widget(entry_search);
		entry_search.set_icon_from_icon_name("search", Gtk::Entry::IconPosition::PRIMARY);
		entry_search.set_placeholder_text("Search");
		entry_search.set_margin(10);
		entry_search.set_size_request(config_main.width - 20, -1);

		entry_search.signal_changed().connect(sigc::mem_fun(*this, &sysmenu::on_search_changed));
		entry_search.signal_activate().connect(sigc::mem_fun(*this, &sysmenu::on_search_done));
		flowbox_itembox.set_sort_func(sigc::mem_fun(*this, &sysmenu::on_sort));
		flowbox_itembox.set_filter_func(sigc::mem_fun(*this, &sysmenu::on_filter));
		entry_search.grab_focus();
	}
	else
		add_controller(controller);

	box_layout.get_style_context()->add_class("layoutbox");
	box_layout.append(scrolled_window);
	scrolled_window.get_style_context()->add_class("innerbox");
	scrolled_window.set_child(flowbox_itembox);

	if (!config_main.scroll_bars)
		scrolled_window.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);

	if (config_main.items_per_row != 1)
		flowbox_itembox.set_halign(Gtk::Align::CENTER);
	flowbox_itembox.set_valign(Gtk::Align::START);
	flowbox_itembox.set_min_children_per_line(config_main.items_per_row);
	flowbox_itembox.set_max_children_per_line(config_main.items_per_row);
	flowbox_itembox.set_vexpand(true);
	flowbox_itembox.signal_child_activated().connect(sigc::mem_fun(*this, &sysmenu::on_child_activated));

	// Load custom css
	std::string home_dir = getenv("HOME");
	std::string css_path = home_dir + "/.config/sys64/menu/style.css";
	css_loader loader(css_path, this);

	// Load applications
	GAppInfoMonitor* app_info_monitor = g_app_info_monitor_get();
	g_signal_connect(app_info_monitor, "changed", G_CALLBACK(+[](GAppInfoMonitor* monitor, gpointer user_data) {
		sysmenu* self = static_cast<sysmenu*>(user_data);
		self->app_info_changed(monitor);
		}), this);

	std::thread thread_appinfo(&sysmenu::app_info_changed, this, nullptr);
	thread_appinfo.detach();
}

bool sysmenu::on_escape_key_press(const guint &keyval, const guint &keycode, const Gdk::ModifierType &state) {
	if (keyval == 65307) // Escape key
		handle_signal(12);
	else if (keyval == 65289) { // Tab
			// TODO: Don't assume child 0 is the first child
			auto children = flowbox_itembox.get_children();
			auto widget = children[0];
			widget->grab_focus();
	}

	return true;
}

void sysmenu::on_search_changed() {
	matches = 0;
	match = "";
	flowbox_itembox.invalidate_filter();
}

// Launch the selected program
void sysmenu::on_child_activated(Gtk::FlowBoxChild* child) {
	launcher *button = dynamic_cast<launcher*>(child->get_child());
	button->on_click();
}

void sysmenu::on_search_done() {
	g_spawn_command_line_async(std::string(match).c_str(), NULL);
	handle_signal(12);
}

bool sysmenu::on_filter(Gtk::FlowBoxChild *child) {
	auto button = dynamic_cast<launcher*> (child->get_child());
	auto text = entry_search.get_text();

	if (button->matches(text)) {
		this->matches++;
		if (matches == 1)
			match = button->app_info->get_executable();
		return true;
	}

	return false;
}

bool sysmenu::on_sort(Gtk::FlowBoxChild* a, Gtk::FlowBoxChild* b) {
	auto b1 = dynamic_cast<launcher*> (a->get_child());
	auto b2 = dynamic_cast<launcher*> (b->get_child());
	return *b2 < *b1;
}

void sysmenu::load_menu_item(Glib::RefPtr<Gio::AppInfo> app_info) {
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

void sysmenu::handle_signal(int signum) {
	switch (signum) {
		case 10: // Showing window
			gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
			get_style_context()->add_class("visible");
			if (config_main.dock_items != "") {
				revealer_search.set_reveal_child(true);
				revealer_dock.set_reveal_child(false);
				box_layout.set_valign(Gtk::Align::FILL);
				box_layout.set_size_request(-1, max_height);
				gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
			}
			else
				show();

			if (config_main.searchbar)
				entry_search.grab_focus();
			break;
		case 12: // Hiding window
			gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
			get_style_context()->remove_class("visible");
			if (config_main.dock_items != "") {
				revealer_search.set_reveal_child(false);
				revealer_dock.set_reveal_child(true);
				box_layout.set_valign(Gtk::Align::END);
				box_layout.set_size_request(-1, -1);
				gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
			}
			else
				hide();

			if (config_main.searchbar)
				entry_search.set_text("");
			break;
		case 34: // Toggling window
			if (config_main.dock_items != "") {
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
}

void sysmenu::on_drag_start(const double &x, const double &y) {
	// For now disable swipe gestures on non touch inputs
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
	int height = box_layout.get_height();

	height = starting_height - y;

	box_layout.set_size_request(-1, height);

	gtk_layer_set_layer(gobj(), (-y < 1) ? GTK_LAYER_SHELL_LAYER_OVERLAY : GTK_LAYER_SHELL_LAYER_BOTTOM);
	revealer_dock.set_reveal_child((-y < 1));
	revealer_search.set_reveal_child((-y > 1));
}

void sysmenu::on_drag_stop(const double &x, const double &y) {
	// For now disable swipe gestures on non touch inputs
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
	sysmenu *sysmenu_create(const config &cfg) {
		return new sysmenu(cfg);
	}
	void sysmenu_handle_signal(sysmenu *window, int signal) {
		window->handle_signal(signal);
	}
}
