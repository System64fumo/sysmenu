#pragma once
#include <string>

// Build time configuration				Description
#define RUNTIME_CONFIG					// Allow the use of runtime arguments

// Default config
struct config {
	bool starthidden = false;
	bool searchbar = true;
	int icon_size = 32;
	int dock_icon_size = 64;
	int app_margin = 4;
	bool name_under_icon = false;
	bool scroll_bars = false;
	ulong max_name_length = 30;
	int items_per_row = 1;
	int width = 400;
	int height = 600;
	int main_monitor = 0;
	bool layer_shell = true;
	bool fill_screen = false;
	std::string dock_items = "";
};
