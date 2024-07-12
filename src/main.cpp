#include "main.hpp"
#include "window.hpp"
#include "config.hpp"
#include "launcher.hpp"
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
	sysmenu_handle_signal_ptr = (sysmenu_handle_signal_func)dlsym(handle, "sysmenu_handle_signal");

	if (!sysmenu_create_ptr || !sysmenu_handle_signal_ptr) {
		std::cerr << "Cannot load symbols: " << dlerror() << '\n';
		dlclose(handle);
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	config config_main;

	#ifdef RUNTIME_CONFIG
	// Read launch arguments
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
