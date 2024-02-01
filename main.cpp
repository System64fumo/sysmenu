#include "main.h"
#include "config.h"
#include <gtkmm.h>
#include <giomm/desktopappinfo.h>
#include <gtk4-layer-shell.h>
#include <gtkmm/cssprovider.h>
#include <gdk/gdkkeysyms.h>
#include <filesystem>
#include <unistd.h>
#include <getopt.h>
#include <cassert>

bool active = !starthidden;
auto app = Gtk::Application::create("funky.sys64.sysmenu");
std::vector<std::shared_ptr<Gio::AppInfo>> app_list;
std::vector<std::unique_ptr<launcher>> items;
std::set<std::pair<std::string, std::string>> loaded_apps;
sysmenu* win = nullptr;

sysmenu::sysmenu() {
	if (layer_shell) {
		gtk_layer_init_for_window(gobj());
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
		gtk_layer_set_namespace(gobj(), "sysmenu");
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);


		if (fill_screen) {
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
			gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, true);
		}
	}

	/* Initialize */
	set_default_size(width, height);
	if (!starthidden)
		show();

	// Events.
	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(
	sigc::mem_fun(*this, &sysmenu::on_escape_key_press), true);
	add_controller(controller);

	/* Layout */
	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_child(box_layout);

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
	}

	box_layout.get_style_context()->add_class("layoutbox");
	box_layout.append(scrolled_window);
	scrolled_window.get_style_context()->add_class("innerbox");
	scrolled_window.set_child(flowbox_itembox);

	// TODO: Consider adding scrollbar controls
	//scrolled_window.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::NEVER);

	if (items_per_row != 1)
		flowbox_itembox.set_halign(Gtk::Align::CENTER);
	flowbox_itembox.set_valign(Gtk::Align::START);
	flowbox_itembox.set_min_children_per_line(items_per_row);
	flowbox_itembox.set_max_children_per_line(items_per_row);
	flowbox_itembox.set_vexpand(true);

	/* Load apps */
	// TODO: Make this async
	for (auto app : app_list)
		load_menu_item(app);

	/* Load css */
	std::string home_dir = getenv("HOME");
	std::string css_path = home_dir + "/.config/sys64/menu.css";
	
	if (!std::filesystem::exists(css_path)) return;

    auto css = Gtk::CssProvider::create();
    css->load_from_path(css_path);
    auto style_context = get_style_context();
    style_context->add_provider_for_display(property_display(), css, GTK_STYLE_PROVIDER_PRIORITY_USER);
}

/* Handle showing or hiding the window */
void handle_signal(int signum) {
	switch (signum) {
		case 10: // Showing window
			if (!win->is_visible())
				win->show();
			
			win->entry_search.grab_focus();
			active = true;
			break;
		case 12: // Hiding window
			if (win->is_visible())
				win->hide();

			win->entry_search.set_text("");
			active = false;
			break;
		case 34: // Toggling window
			if (active)
				handle_signal(12);
			else
				handle_signal(10);
			break;
	}
}

bool sysmenu::on_escape_key_press(guint keyval, guint, Gdk::ModifierType state) {
	if (keyval == 65307) // Escape key
		handle_signal(12);
	return true;
}

/* Item loading */
launcher::launcher(AppInfo app) : Gtk::Box(), app_info(app) {
	Glib::ustring name = app->get_display_name();
	Glib::ustring description = app->get_description();

	if (items_per_row == 1)
		box_launcher.set_margin_top(app_margin);
	else
		box_launcher.set_margin(app_margin);

	image_program.set(app->get_icon());
	image_program.set_pixel_size(icon_size);

	set_tooltip_text(description);

	if (name.length() > max_name_length)
		name = name.substr(0, max_name_length - 2) + "..";

	label_program.set_text(name);
	label_program.property_margin_start().set_value(5);

	int hr = -1;
	int wr = -1;
	if (name_under_icon) {
		box_launcher.set_orientation(Gtk::Orientation::VERTICAL);
		box_launcher.set_valign(Gtk::Align::CENTER);
		button_launcher.set_valign(Gtk::Align::START);
		hr = max_name_length * 10;
		wr = max_name_length * 10;
	}

	box_launcher.append(image_program);
	box_launcher.append(label_program);

	button_launcher.set_child(box_launcher);

	append(button_launcher);

	button_launcher.set_hexpand(true);
	button_launcher.set_size_request(wr, hr);

	button_launcher.get_style_context()->add_class("flat");
	button_launcher.signal_clicked().connect(sigc::mem_fun(*this, &launcher::on_click));
}

void launcher::on_click() {
	app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
	handle_signal(12);
}

/* Search */
void sysmenu::on_search_changed() {
	count_matches = 0;
	first_match = "";
	flowbox_itembox.invalidate_filter();
}

void sysmenu::on_search_done() {
	// Launch the most relevant program on Enter key press.
	// TODO: Add the ability to use arrow keys to select different search results
	g_spawn_command_line_async(std::string(first_match).c_str(), NULL);
	handle_signal(12);
}

bool sysmenu::on_filter(Gtk::FlowBoxChild *child) {
	auto button = dynamic_cast<launcher*> (child->get_child());
	assert(button);

	auto text = entry_search.get_text();

	if (button->matches(text)) {
		this->count_matches++;
		if (count_matches == 1) {
			first_match = button->app_info->get_executable();
		}
		return true;
	}

	return false;
}

bool sysmenu::on_sort(Gtk::FlowBoxChild* a, Gtk::FlowBoxChild* b) {
	auto b1 = dynamic_cast<launcher*> (a->get_child());
	auto b2 = dynamic_cast<launcher*> (b->get_child());
	assert(b1 && b2);

	return *b2 < *b1;
}

bool launcher::matches(Glib::ustring pattern) {
	Glib::ustring name = app_info->get_name();
	Glib::ustring long_name = app_info->get_display_name();
	Glib::ustring progr = app_info->get_executable();
	Glib::ustring descr = app_info->get_description();

	Glib::ustring text = name.lowercase() + "$"
		+ long_name.lowercase() + "$" + progr.lowercase() + "$"
		+ descr.lowercase();

	return text.find(pattern.lowercase()) != text.npos;
}

bool launcher::operator < (const launcher& other) {
	return Glib::ustring(app_info->get_name()).lowercase()
		< Glib::ustring(other.app_info->get_name()).lowercase();
}

void sysmenu::load_menu_item(AppInfo app_info) {
	if (!app_info || !app_info->should_show())
		return;

	auto name = app_info->get_name();
	auto exec = app_info->get_executable();
	/* If we don't have the following, then the entry won't be useful anyway,
	 * so we should skip it */
	if (name.empty() || !app_info->get_icon() || exec.empty())
		return;

	/* Already created such a launcher, skip */
	if (loaded_apps.count({name, exec}))
		return;
	loaded_apps.insert({name, exec});

	items.push_back(std::unique_ptr<launcher>(new launcher(app_info)));
	flowbox_itembox.append(*items.back());
}

int main(int argc, char* argv[]) {

	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "Ssi:dm:dun:dp:dW:dH:dlfh")) {
			case 'S':
				starthidden=true;
				active=false;
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
				printf("  -n	Max name length\n");
				printf("  -p	Items per row\n");
				printf("  -W	Set window width\n");
				printf("  -H	Set window Height\n");
				printf("  -l	Disable use of layer shell\n");
				printf("  -f	Fullscreen\n");
				printf("  -h	Show this help message\n");
				return 0;

			case -1:
				break;
			}

			break;
	}

	// Handle signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	app_list = Gio::AppInfo::get_all();

	win = new sysmenu();
	win->entry_search.grab_focus();
	
	win->set_hide_on_close(true);
	app->hold();
	
	return app->run();
}
