#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "launcher.hpp"

#include <giomm/desktopappinfo.h>
#include <gtk4-layer-shell.h>
#include <getopt.h>
#include <iostream>
#include <thread>

//std::thread thread_height;

/* Handle showing or hiding the window */
void handle_signal(int signum) {
	switch (signum) {
		case 10: // Showing window
			win->get_style_context()->add_class("visible");
			if (gestures) {
				win->centerbox_top.set_visible(true);
				win->box_layout.set_valign(Gtk::Align::FILL);
				win->box_layout.set_size_request(-1, win->max_height);
				gtk_layer_set_anchor(win->gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);

				// I am sure there's a better way of doing this,
				// But whare are you gonna do? Make a commit?
				/*if (thread_height.joinable())
					thread_height.join();
				thread_height = std::thread(win->get_window_height);*/
			}
			else
				win->show();

			if (searchbar)
				win->entry_search.grab_focus();
			break;
		case 12: // Hiding window
			win->get_style_context()->remove_class("visible");
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
				win->starting_height = win->box_layout.get_height();
				if (win->box_layout.get_height() < win->max_height / 2)
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
				std::cout << "usage:" << std::endl;
				std::cout << "  sysmenu [argument...]:\n" << std::endl;
				std::cout << "arguments:" << std::endl;
				std::cout << "  -S	Hide the program on launch" << std::endl;
				std::cout << "  -s	Hide the search bar" << std::endl;
				std::cout << "  -i	Set launcher icon size" << std::endl;
				std::cout << "  -m	Set launcher margins" << std::endl;
				std::cout << "  -u	Show name under icon" << std::endl;
				std::cout << "  -b	Show scroll bars" << std::endl;
				std::cout << "  -n	Max name length" << std::endl;
				std::cout << "  -p	Items per row" << std::endl;
				std::cout << "  -W	Set window width" << std::endl;
				std::cout << "  -H	Set window Height" << std::endl;
				std::cout << "  -l	Disable use of layer shell" << std::endl;
				std::cout << "  -g	Enable touchscreen swipe gesture (Experimental)" << std::endl;
				std::cout << "  -f	Fullscreen" << std::endl;
				std::cout << "  -h	Show this help message" << std::endl;
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
	g_signal_connect(app_info_monitor, "changed", G_CALLBACK(win->app_info_changed), nullptr);
	win->app_info_changed(nullptr, nullptr);

	return app->run();
}
