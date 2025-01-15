#include "main.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <gtkmm/application.h>
#include <filesystem>
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
	std::string config_path;
	std::map<std::string, std::map<std::string, std::string>> config;
	std::map<std::string, std::map<std::string, std::string>> config_usr;

	bool cfg_sys = std::filesystem::exists("/usr/share/sys64/menu/config.conf");
	bool cfg_sys_local = std::filesystem::exists("/usr/local/share/sys64/menu/config.conf");
	bool cfg_usr = std::filesystem::exists(std::string(getenv("HOME")) + "/.config/sys64/menu/config.conf");

	// Load default config
	if (cfg_sys)
		config_path = "/usr/share/sys64/menu/config.conf";
	else if (cfg_sys_local)
		config_path = "/usr/local/share/sys64/menu/config.conf";
	else
		std::fprintf(stderr, "No default config found, Things will get funky!\n");

	config = config_parser(config_path).data;

	// Load user config
	if (cfg_usr)
		config_path = std::string(getenv("HOME")) + "/.config/sys64/menu/config.conf";
	else
		std::fprintf(stderr, "No user config found\n");

	config_usr = config_parser(config_path).data;

	// Merge configs
	for (const auto& [key, nested_map] : config_usr)
		for (const auto& [inner_key, inner_value] : nested_map)
			config[key][inner_key] = inner_value;

	// Sanity check
	if (!(cfg_sys || cfg_sys_local || cfg_usr)) {
		std::fprintf(stderr, "No config available, Something ain't right here.");
		return 1;
	}
	#endif

	// Read launch arguments
	#ifdef CONFIG_RUNTIME
	while (true) {
		switch(getopt(argc, argv, "Ssi:I:m:ubn:p:a:W:H:M:l:D:vhLP:d")) {
			case 'S':
				config["main"]["start-hidden"] = "true";
				continue;

			case 's':
				config["main"]["searchbar"] = "false";
				continue;

			case 'i':
				config["main"]["icon-size"] = optarg;
				continue;

			case 'I':
				config["main"]["dock-icon-size"] = optarg;
				continue;

			case 'm':
				config["main"]["app-margin"] = optarg;
				continue;

			case 'u':
				config["main"]["name-under-icon"] = "true";
				continue;

			case 'b':
				config["main"]["scroll-bars"] = "true";
				continue;

			case 'n':
				config["main"]["name-length"] = optarg;
				continue;

			case 'P':
				config["main"]["items-per-row"] = optarg;
				continue;

			case 'p':
				config["main"]["prompt"] = optarg;
				continue;

			case 'a':
				config["main"]["anchors"] = optarg;
				continue;

			case 'W':
				config["main"]["width"] = optarg;
				continue;

			case 'H':
				config["main"]["height"] = optarg;
				continue;

			case 'M':
				config["main"]["monitor"] = optarg;
				continue;

			// -l sets the number of lines in dmenu scripts
			// there is no trivial way to implement it, but ignoring it is an option
			case 'l':
				break;

			case 'L':
				config["main"]["layer-shell"] = "false";
				continue;

			case 'D':
				config["main"]["dock-items"] = optarg;
				config["main"]["layer-shell"] = "true";
				config["main"]["anchors"] = "top right bottom left";
				continue;

			case 'd':
				config["main"]["dmenu"] = "true";
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
				std::cout << "  -p	Set placeholder text" << std::endl;
				std::cout << "  -P	Items per row" << std::endl;
				std::cout << "  -a	Set anchors" << std::endl;
				std::cout << "  -W	Set window width" << std::endl;
				std::cout << "  -H	Set window Height" << std::endl;
				std::cout << "  -M	Set primary monitor" << std::endl;
				std::cout << "  -L	Disable use of layer shell" << std::endl;
				std::cout << "  -d	dmenu emulation" << std::endl;
				std::cout << "  -D	Set dock items" << std::endl;
				std::cout << "  -v	Prints version info" << std::endl;
				std::cout << "  -h	Show this help message" << std::endl;
				return 0;

			case -1:
				break;
			}

			break;
	}

	if ( config["main"]["dmenu"] == "true" ) {
		config["main"]["start-hidden"] = "false";
	}
	#endif

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create("funky.sys64.sysmenu");
	app->hold();

	load_libsysmenu();
	win = sysmenu_create_ptr(config);

	// Catch signals if not in dmenu mode
	if (config["main"]["dmenu"] != "true") {
		signal(SIGUSR1, handle_signal);
		signal(SIGUSR2, handle_signal);
		signal(SIGRTMIN, handle_signal);
	}

	return app->run();
}
