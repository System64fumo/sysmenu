#pragma once
#include <string>

// Build time configuration				Description
#define CONFIG_RUNTIME					// Allow the use of runtime arguments
#define CONFIG_FILE						// Allow the use of a config file

// Default config
struct config_menu {
	bool starthidden = false;
	bool searchbar = true;
	int icon_size = 32;
	int dock_icon_size = 64;
	int app_margin = 4;
	bool name_under_icon = false;
	bool scroll_bars = false;
	ulong max_name_length = 30;
	int items_per_row = 1;
	std::string anchors = "";
	int width = 400;
	int height = 600;
	int main_monitor = 0;
	bool layer_shell = true;
	std::string dock_items = "";
};
