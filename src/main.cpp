#include "main.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <gtkmm/application.h>
#include <iostream>
#include <dlfcn.h>

void handle_signal(int signum) {
	sysmenu_handle_signal_ptr(win, signum);
}

void load_libsysmenu() {
	void* handle = dlopen("libsysmenu.so", RTLD_LAZY);
	if (!handle) {
		std::cerr << "Cannot open library: " << dlerror() << '\n';
		exit(1);
	}

	sysmenu_create_ptr = (sysmenu_create_func)dlsym(handle, "sysmenu_create");
	sysmenu_handle_signal_ptr = (sysmenu_handle_signal_func)dlsym(handle, "sysmenu_signal");

	if (!sysmenu_create_ptr || !sysmenu_handle_signal_ptr) {
		std::cerr << "Cannot load symbols: " << dlerror() << '\n';
		dlclose(handle);
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	// Load the config
	#ifdef CONFIG_FILE
	config_parser config(std::string(getenv("HOME")) + "/.config/sys64/menu/config.conf");

	if (config.available) {
		std::string cfg_start_hidden = config.get_value("main", "start-hidden");
		if (cfg_start_hidden != "empty")
			config_main.starthidden = (cfg_start_hidden == "true");

		std::string cfg_searchbar = config.get_value("main", "searchbar");
		if (cfg_searchbar != "empty")
			config_main.searchbar = (cfg_searchbar == "true");

		std::string cfg_icon_size = config.get_value("main", "icon-size");
		if (cfg_icon_size != "empty")
			config_main.icon_size = std::stoi(cfg_icon_size);

		std::string cfg_dock_icon_size = config.get_value("main", "dock-icon-size");
		if (cfg_dock_icon_size != "empty")
			config_main.dock_icon_size = std::stoi(cfg_dock_icon_size);

		std::string cfg_app_margins = config.get_value("main", "app-margins");
		if (cfg_app_margins != "empty")
			config_main.app_margin = std::stoi(cfg_app_margins);

		std::string cfg_name_under_icon = config.get_value("main", "name-under-icon");
		if (cfg_name_under_icon != "empty")
			config_main.name_under_icon = (cfg_name_under_icon == "true");

		std::string cfg_scroll_bars = config.get_value("main", "scroll-bars");
		if (cfg_scroll_bars != "empty")
			config_main.scroll_bars = (cfg_scroll_bars == "true");

		std::string cfg_name_length = config.get_value("main", "name-length");
		if (cfg_name_length != "empty")
			config_main.max_name_length = std::stoi(cfg_name_length);

		std::string cfg_items_per_row = config.get_value("main", "items-per-row");
		if (cfg_items_per_row != "empty")
			config_main.items_per_row = std::stoi(cfg_items_per_row);

		std::string cfg_width = config.get_value("main", "width");
		if (cfg_width != "empty")
			config_main.width = std::stoi(cfg_width);

		std::string cfg_height =  config.get_value("main", "height");
		if (cfg_height != "empty")
			config_main.height=std::stoi(cfg_height);

		std::string cfg_monitor =  config.get_value("main", "monitor");
		if (cfg_monitor != "empty")
			config_main.main_monitor=std::stoi(cfg_monitor);

		std::string cfg_layer_shell =  config.get_value("main", "layer-shell");
		if (cfg_layer_shell != "empty")
			config_main.layer_shell = (cfg_layer_shell == "true");

		std::string cfg_fullscreen =  config.get_value("main", "fullscreen");
		if (cfg_fullscreen != "empty")
			config_main.fill_screen = (cfg_fullscreen == "true");

		std::string cfg_dock_items =  config.get_value("main", "dock-items");
		if (cfg_dock_items != "empty" && !cfg_dock_items.empty()) {
			config_main.dock_items = cfg_dock_items;
			config_main.layer_shell = true;
			config_main.fill_screen = true;
		}
	}
	#endif

	// Read launch arguments
	#ifdef CONFIG_RUNTIME
	while (true) {
		switch(getopt(argc, argv, "Ssi:dI:dm:dubn:dp:dW:dH:dM:dlD:Sfvh")) {
			case 'S':
				config_main.starthidden=true;
				continue;

			case 's':
				config_main.searchbar=false;
				continue;

			case 'i':
				config_main.icon_size=std::stoi(optarg);
				continue;

			case 'I':
				config_main.dock_icon_size=std::stoi(optarg);
				continue;

			case 'm':
				config_main.app_margin=std::stoi(optarg);
				continue;

			case 'u':
				config_main.name_under_icon=true;
				continue;

			case 'b':
				config_main.scroll_bars=true;
				continue;

			case 'n':
				config_main.max_name_length=std::stoi(optarg);
				continue;

			case 'p':
				config_main.items_per_row=std::stoi(optarg);
				continue;

			case 'W':
				config_main.width=std::stoi(optarg);
				continue;

			case 'H':
				config_main.height=std::stoi(optarg);
				continue;

			case 'M':
				config_main.main_monitor=std::stoi(optarg);
				continue;

			case 'l':
				config_main.layer_shell=false;
				continue;

			case 'D':
				config_main.dock_items=optarg;
				config_main.layer_shell=true;
				config_main.fill_screen=true;
				continue;

			case 'f':
				config_main.fill_screen=true;
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
				std::cout << "  -I	Set dock icon size" << std::endl;
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

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("funky.sys64.sysmenu");
	app->hold();

	load_libsysmenu();
	win = sysmenu_create_ptr(config_main);

	// Catch signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	return app->run();
}
