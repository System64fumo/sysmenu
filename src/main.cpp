#include "main.hpp"
#include "config.hpp"
#include "launcher.hpp"
#include <giomm/desktopappinfo.h>
#include <gtk4-layer-shell.h>
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
