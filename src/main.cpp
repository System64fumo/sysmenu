#include "main.hpp"
#include "config.hpp"
#include "config_parser.hpp"
#include "git_info.hpp"

#include <filesystem>
#include <dlfcn.h>
#include <unistd.h> 
#include <signal.h> 

QApplication* app = nullptr;
sysmenu* win = nullptr;

void handle_signal(int signum) {
	sysmenu_handle_signal_ptr(win, signum);
}

void load_libsysmenu() {
	void* handle = dlopen("libsysmenu.so", RTLD_LAZY);
	if (!handle) {
		std::fprintf(stderr, "Cannot open library: %s\n", dlerror());
		exit(1);
	}

	sysmenu_create_ptr = (sysmenu_create_func)dlsym(handle, "sysmenu_create");
	sysmenu_handle_signal_ptr = (sysmenu_handle_signal_func)dlsym(handle, "sysmenu_signal");

	if (!sysmenu_create_ptr || !sysmenu_handle_signal_ptr) {
		std::fprintf(stderr, "Cannot load symbols: %s\n", dlerror());
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
		switch(getopt(argc, argv, "Ssi:m:ubn:p:a:W:H:M:vh")) {
			case 'S':
				config["main"]["start-hidden"] = "true";
				continue;

			case 's':
				config["main"]["searchbar"] = "false";
				continue;

			case 'i':
				config["main"]["icon-size"] = optarg;
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

			case 'p':
				config["main"]["items-per-row"] = optarg;
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

			case 'v':
				std::printf("Commit: %s\n", GIT_COMMIT_MESSAGE);
				std::printf("Date: %s\n", GIT_COMMIT_DATE);
				return 0;

			case 'h':
			default :
				std::printf("usage:\n");
				std::printf("  sysmenu [argument...]:\n\n");
				std::printf("arguments:\n");
				std::printf("  -S	Hide the program on launch\n");
				std::printf("  -s	Hide the search bar\n");
				std::printf("  -i	Set launcher icon size\n");
				std::printf("  -m	Set launcher margins\n");
				std::printf("  -u	Show name under icon\n");
				std::printf("  -b	Show scroll bars\n");
				std::printf("  -n	Max name length\n");
				std::printf("  -p	Items per row\n");
				std::printf("  -a	Set anchors\n");
				std::printf("  -W	Set window width\n");
				std::printf("  -H	Set window Height\n");
				std::printf("  -M	Set primary monitor\n");
				std::printf("  -v	Prints version info\n");
				std::printf("  -h	Show this help message\n");
				return 0;

			case -1:
				break;
			}

			break;
	}
	#endif

	app = new QApplication(argc, argv);

	load_libsysmenu();
	win = sysmenu_create_ptr(config);

	// Catch signals
	signal(SIGUSR1, handle_signal);
	signal(SIGUSR2, handle_signal);
	signal(SIGRTMIN, handle_signal);

	return app->exec();
}
