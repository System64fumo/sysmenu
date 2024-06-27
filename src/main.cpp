#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "launcher.hpp"
#include "git_info.hpp"

#include <gtk4-layer-shell.h>
#include <iostream>
#include <thread>

/* Handle showing or hiding the window */
void handle_signal(int signum) {
	switch (signum) {
		case 10: // Showing window
			gtk_layer_set_layer(win->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
			win->get_style_context()->add_class("visible");
			if (dock_items != "") {
				win->revealer_search.set_reveal_child(true);
				win->revealer_dock.set_reveal_child(false);
				win->box_layout.set_valign(Gtk::Align::FILL);
				win->box_layout.set_size_request(-1, win->max_height);
				gtk_layer_set_anchor(win->gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
			}
			else
				win->show();

			if (searchbar)
				win->entry_search.grab_focus();
			break;
		case 12: // Hiding window
			gtk_layer_set_layer(win->gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
			win->get_style_context()->remove_class("visible");
			if (dock_items != "") {
				win->revealer_search.set_reveal_child(false);
				win->revealer_dock.set_reveal_child(true);
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
			if (dock_items != "") {
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

	#ifdef RUNTIME_CONFIG
	// Read launch arguments
	while (true) {
		switch(getopt(argc, argv, "Ssi:dm:dubn:dp:dW:dH:dM:dlD:Sfvh")) {
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
				width=std::stoi(optarg);
				continue;

			case 'H':
				height=std::stoi(optarg);
				continue;

			case 'M':
				main_monitor=std::stoi(optarg);
				continue;

			case 'l':
				layer_shell=false;
				continue;

			case 'D':
				dock_items=optarg;
				layer_shell=true;
				fill_screen=true;
				continue;

			case 'f':
				fill_screen=true;
				continue;

			case 'v':
				std::cout << "Commit: " << GIT_COMMIT_MESSAGE << std::endl;
				std::cout << "Date: " << GIT_COMMIT_DATE << std::endl;
				return 0;

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
				std::cout << "  -M	Set primary monitor" << std::endl;
				std::cout << "  -l	Disable use of layer shell" << std::endl;
				std::cout << "  -D	Set dock items" << std::endl;
				std::cout << "  -f	Fullscreen" << std::endl;
				std::cout << "  -v	Prints version info" << std::endl;
				std::cout << "  -h	Show this help message" << std::endl;
				return 0;

			case -1:
				break;
			}

			break;
	}
	#endif

	// Catch signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	app = Gtk::Application::create("funky.sys64.sysmenu");
	app->hold();
	win = new sysmenu();

	return app->run();
}
