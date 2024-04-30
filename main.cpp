#include "main.hpp"
#include "config.hpp"
#include <giomm/desktopappinfo.h>
#include <gtk4-layer-shell.h>
#include <gtkmm/cssprovider.h>
#include <filesystem>
#include <getopt.h>
#include <thread>

std::vector<std::shared_ptr<Gio::AppInfo>> app_list;
std::vector<std::unique_ptr<launcher>> items;
sysmenu* win;

std::thread thread_height;
int starting_height = 0;
int max_height = 100; // TODO: Assume initial max size of the window (Use screen height)

void get_window_height() {
	// This is required because otherwise we get the current
	// height of the window rather than getting the max size of it.
	g_usleep(100000);
	max_height = win->get_height();
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

	// Events 
	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(
	sigc::mem_fun(*this, &sysmenu::on_escape_key_press), true);
	add_controller(controller);

	if (searchbar) {
		entry_search.get_style_context()->add_class("searchbar");
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
	scrolled_window.get_style_context()->add_class("visible");
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

/* Launchers */
launcher::launcher(AppInfo app) : Gtk::Button(), app_info(app) {
	Glib::ustring name = app->get_display_name();
	Glib::ustring description = app->get_description();

	if (items_per_row == 1)
		set_margin_top(app_margin);
	else
		set_margin(app_margin);

	image_program.set(app->get_icon());
	image_program.set_pixel_size(icon_size);

	set_tooltip_text(description);

	if (name.length() > max_name_length)
		name = name.substr(0, max_name_length - 2) + "..";

	label_program.set_text(name);

	int size_request = -1;
	if (name_under_icon) {
		box_launcher.set_orientation(Gtk::Orientation::VERTICAL);
		box_launcher.set_valign(Gtk::Align::CENTER);
		set_valign(Gtk::Align::START);
		size_request = max_name_length * 10;
	}
	else
		label_program.property_margin_start().set_value(10);

	box_launcher.append(image_program);
	box_launcher.append(label_program);

	set_child(box_launcher);

	set_hexpand(true);
	set_size_request(size_request, size_request);

	get_style_context()->add_class("flat");
	signal_clicked().connect(sigc::mem_fun(*this, &launcher::on_click));
}

void launcher::on_click() {
	app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
	handle_signal(12);
}

/* Search */
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

bool launcher::matches(Glib::ustring pattern) {
	Glib::ustring name = app_info->get_name();
	Glib::ustring long_name = app_info->get_display_name();
	Glib::ustring progr = app_info->get_executable();
	Glib::ustring descr = app_info->get_description();

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

static void app_info_changed(GAppInfoMonitor* gappinfomonitor, gpointer user_data) {
	app_list = Gio::AppInfo::get_all();
	win->flowbox_itembox.remove_all();

	// Load applications
	for (auto app : app_list)
		win->load_menu_item(app);
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

/* Handle showing or hiding the window */
void handle_signal(int signum) {
	switch (signum) {
		case 10: // Showing window
			win->box_layout.get_style_context()->add_class("visible");
			if (gestures) {
				win->centerbox_top.set_visible(true);
				win->box_layout.set_valign(Gtk::Align::FILL);
				win->box_layout.set_size_request(-1, max_height);
				gtk_layer_set_anchor(win->gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);

				// I am sure there's a better way of doing this,
				// But whare are you gonna do? Make a commit?
				if (thread_height.joinable())
					thread_height.join();
				thread_height = std::thread(get_window_height);
			}
			else
				win->show();

			if (searchbar)
				win->entry_search.grab_focus();
			break;
		case 12: // Hiding window
			win->box_layout.get_style_context()->remove_class("visible");
			if (gestures) {
				win->centerbox_top.set_visible(false);
				win->box_layout.set_valign(Gtk::Align::END);
				win->box_layout.set_size_request(-1, -1);
				gtk_layer_set_anchor(win->gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
			}
			else
				win->hide();

			if (searchbar)
				win->entry_search.set_text("");
			break;
		case 34: // Toggling window
			if (gestures) {
				starting_height = win->box_layout.get_height();
				if (win->box_layout.get_height() < max_height / 2)
					handle_signal(10);
				else
					handle_signal(12);
			}
			else {
				if (win->is_visible())
					handle_signal(12);
				else
					handle_signal(10);
			}
			break;
	}
}

int main(int argc, char* argv[]) {

	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "Ssi:dm:dubn:dp:dW:dH:dlgfh")) {
			case 'S':
				starthidden=true;
				continue;

			case 's':
				searchbar=false;
				continue;

			case 'i':
				icon_size=std::stoi(optarg);
				continue;

			case 'm':
				app_margin=std::stoi(optarg);
				continue;

			case 'u':
				name_under_icon=true;
				continue;

			case 'b':
				scroll_bars=true;
				continue;

			case 'n':
				max_name_length=std::stoi(optarg);
				continue;

			case 'p':
				items_per_row=std::stoi(optarg);
				continue;

			case 'W':
				width=std::stoi(optarg);;
				continue;

			case 'H':
				height=std::stoi(optarg);;
				continue;

			case 'l':
				layer_shell=false;
				continue;

			case 'g':
				gestures=true;

				// Set these because we need them
				layer_shell=true;
				fill_screen=true;
				continue;

			case 'f':
				fill_screen=true;
				continue;

			case 'h':
			default :
				printf("usage:\n");
				printf("  sysmenu [argument...]:\n\n");
				printf("arguments:\n");
				printf("  -S	Hide the program on launch\n");
				printf("  -s	Hide the search bar\n");
				printf("  -i	Set launcher icon size\n");
				printf("  -m	Set launcher margins\n");
				printf("  -u	Show name under icon\n");
				printf("  -b	Show scroll bars\n");
				printf("  -n	Max name length\n");
				printf("  -p	Items per row\n");
				printf("  -W	Set window width\n");
				printf("  -H	Set window Height\n");
				printf("  -l	Disable use of layer shell\n");
				printf("  -g	Enable touchscreen swipe gesture (Experimental)\n");
				printf("  -f	Fullscreen\n");
				printf("  -h	Show this help message\n");
				return 0;

			case -1:
				break;
			}

			break;
	}

	// Catch signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	app = Gtk::Application::create("funky.sys64.sysmenu");
	app->hold();
	win = new sysmenu();

	// TODO: Set max_height from the display's resolution as a fallback

	GAppInfoMonitor* app_info_monitor = g_app_info_monitor_get();
	g_signal_connect(app_info_monitor, "changed", G_CALLBACK(app_info_changed), nullptr);
	app_info_changed(nullptr, nullptr);

	return app->run();
}
