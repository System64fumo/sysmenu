#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "launcher.hpp"

#include <giomm/desktopappinfo.h>
#include <gtk4-layer-shell.h>
#include <gtkmm/cssprovider.h>
#include <filesystem>

using AppInfo = Glib::RefPtr<Gio::AppInfo>;
std::vector<std::shared_ptr<Gio::AppInfo>> app_list;
std::vector<std::unique_ptr<launcher>> items;

void sysmenu::app_info_changed(GAppInfoMonitor* gappinfomonitor, gpointer user_data) {
	app_list = Gio::AppInfo::get_all();
	win->flowbox_itembox.remove_all();

	// Load applications
	for (auto app : app_list)
		win->load_menu_item(app);
}

sysmenu::sysmenu() {
	if (layer_shell) {
		gtk_layer_init_for_window(gobj());
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
		gtk_layer_set_namespace(gobj(), "sysmenu");
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);

		if (fill_screen) {
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
			if (!gestures)
				gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
		}
	}

	// Experimental if you couldn't guess by the many TODOs (I am lazy)
	if (gestures) {
		// TODO: Gestures currently have issues on non touchscreen inputs,
		// Ideally this should be fixed..
		// Buuuuuuut the better solution is to just disable non touchscreen input
		// events from interacting with the drag gesture if possible.

		// TODO: Dragging causes the inner scrollbox to resize, This is bad as
		// it uses a lot of cpu power trying to resize things.

		// TODO: Add a proper separator and option to not make the bar span the
		// bottom edge of the screen.

		// TODO: Swipe gestures don't work? maybe add those one day?
		// like.. swiping from the screen edge does not trigger the gesture.

		// Set up gestures
		gesture_drag = Gtk::GestureDrag::create();
		gesture_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &sysmenu::on_drag_start));
		gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysmenu::on_drag_update));
		gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysmenu::on_drag_stop));

		box_layout.append(box_grabber);
		box_grabber.property_orientation().set_value(Gtk::Orientation::VERTICAL);
		box_grabber.get_style_context()->add_class("grabber");
		box_grabber.add_controller(gesture_drag);
		box_grabber.set_size_request(-1, 30);

		// Set window height to the grabber's height
		height = box_grabber.property_height_request().get_value();
		box_layout.set_valign(Gtk::Align::END);
		centerbox_top.set_visible(false);
	}

	// Initialize
	set_default_size(width, height);
	set_hide_on_close(true);
	if (!starthidden)
		show();
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_child(box_layout);

	// Max screen height
	// TODO: Make this code better
	// Sadly there does not seem to be a way to detect what the default monitor is
	// Gotta assume or ask the user for their monitor of choice
	GdkDisplay *display = gdk_display_get_default();
	GListModel *monitors = gdk_display_get_monitors(display);
	GdkMonitor *monitor = GDK_MONITOR(g_list_model_get_item(monitors, main_monitor));
	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);
	max_height = geometry.height;

	// Events 
	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(
	sigc::mem_fun(*this, &sysmenu::on_escape_key_press), true);
	add_controller(controller);

	if (searchbar) {
		entry_search.get_style_context()->add_class("searchbar");
		if (gestures)
			box_grabber.append(centerbox_top);
		else
			box_layout.append(centerbox_top);
		centerbox_top.set_center_widget(entry_search);
		entry_search.set_icon_from_icon_name("search", Gtk::Entry::IconPosition::PRIMARY);
		entry_search.set_placeholder_text("Search");
		entry_search.set_margin(10);
		entry_search.set_size_request(width - 20, -1);

		entry_search.signal_changed().connect(sigc::mem_fun(*this, &sysmenu::on_search_changed));
		entry_search.signal_activate().connect(sigc::mem_fun(*this, &sysmenu::on_search_done));
		flowbox_itembox.set_sort_func(sigc::mem_fun(*this, &sysmenu::on_sort));
		flowbox_itembox.set_filter_func(sigc::mem_fun(*this, &sysmenu::on_filter));
		entry_search.grab_focus();
	}

	box_layout.get_style_context()->add_class("layoutbox");
	box_layout.append(scrolled_window);
	scrolled_window.get_style_context()->add_class("innerbox");
	scrolled_window.set_child(flowbox_itembox);

	if (!scroll_bars)
		scrolled_window.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);

	if (items_per_row != 1)
		flowbox_itembox.set_halign(Gtk::Align::CENTER);
	flowbox_itembox.set_valign(Gtk::Align::START);
	flowbox_itembox.set_min_children_per_line(items_per_row);
	flowbox_itembox.set_max_children_per_line(items_per_row);
	flowbox_itembox.set_vexpand(true);

	// Load custom css
	std::string home_dir = getenv("HOME");
	std::string css_path = home_dir + "/.config/sys64/menu.css";

	if (!std::filesystem::exists(css_path)) return;

	auto css = Gtk::CssProvider::create();
	css->load_from_path(css_path);
	auto style_context = get_style_context();
	style_context->add_provider_for_display(property_display(), css, GTK_STYLE_PROVIDER_PRIORITY_USER);
}

bool sysmenu::on_escape_key_press(guint keyval, guint, Gdk::ModifierType state) {
	if (keyval == 65307) // Escape key
		handle_signal(12);
	return true;
}

void sysmenu::on_search_changed() {
	matches = 0;
	match = "";
	flowbox_itembox.invalidate_filter();
}

void sysmenu::on_search_done() {
	// Launch the most relevant program on Enter key press.
	// TODO: Add the ability to use arrow keys to select different search results
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

void sysmenu::load_menu_item(AppInfo app_info) {
	if (!app_info || !app_info->should_show() || !app_info->get_icon())
		return;

	auto name = app_info->get_name();
	auto exec = app_info->get_executable();

	// Skip loading empty entries
	if (name.empty() || exec.empty())
		return;

	items.push_back(std::unique_ptr<launcher>(new launcher(app_info)));
	flowbox_itembox.append(*items.back());
}

void sysmenu::on_drag_start(int x, int y) {
	centerbox_top.set_visible(false);
	starting_height = box_layout.get_height();
	gtk_layer_set_layer(win->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
	box_layout.set_valign(Gtk::Align::END);
}

void sysmenu::on_drag_update(int x, int y) {
	int height = box_layout.get_height();

	// TODO: There's probably a better way of doing this.
	if (starting_height > (max_height / 2))
		height = starting_height - y;
	else
		height = height + (-y + box_grabber.property_height_request().get_value()) - win->get_height();

	box_layout.set_size_request(-1, height);
}

void sysmenu::on_drag_stop(int x, int y) {
	// Top position
	if (box_layout.get_height() > max_height / 2)
		handle_signal(10);
	// Bottom Position
	else
		handle_signal(12);
}
