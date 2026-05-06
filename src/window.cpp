#include "main.hpp"
#include "window.hpp"
#include "config.hpp"

#include <gtkmm/cssprovider.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtk4-layer-shell.h>
#include <thread>
#include <filesystem>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <gtkmm/adjustment.h>
#include <algorithm>
#include <gdk/gdkkeysyms.h>
#include <signal.h>

sysmenu::sysmenu(const std::map<std::string, std::map<std::string, std::string>>& cfg) : config_main(cfg) {
	bool edge_top = config_main["main"]["anchors"].find("top") != std::string::npos;
	bool edge_right = config_main["main"]["anchors"].find("right") != std::string::npos;
	bool edge_bottom = config_main["main"]["anchors"].find("bottom") != std::string::npos;
	bool edge_left = config_main["main"]["anchors"].find("left") != std::string::npos;
	is_dock = !config_main["main"]["dock-items"].empty();
	has_searchbar = config_main["main"]["searchbar"] == "true";

	if (is_dock && !(edge_top && edge_right && edge_bottom && edge_left)) {
		std::fprintf(stderr, "Warning: Dock mode requires all anchors (top, right, bottom, left). Forcing fullscreen mode.\n");
		edge_top = edge_right = edge_bottom = edge_left = true;
		config_main["main"]["anchors"] = "top right bottom left";
	}

	if (config_main["main"]["layer-shell"] == "true") {
		gtk_layer_init_for_window(gobj());
		gtk_layer_set_keyboard_mode(gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
		gtk_layer_set_namespace(gobj(), "sysmenu");

		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, edge_top && !is_dock);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, edge_right);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, edge_bottom);
		gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_LEFT, edge_left);
	}

	add_css_class((edge_top && edge_right && edge_bottom && edge_left) ? "fullscreen" : "window");

	if (is_dock) {
		add_css_class("dock");
		remove_css_class("background");
		sysmenu_dock = Gtk::make_managed<dock>(config_main);
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
		// TODO: Dragging causes the inner scrollbox to resize, This is bad as
		// it uses a lot of cpu power trying to resize things.
		// Is this even possible to fix?

		// Set up gestures
		gesture_drag = Gtk::GestureDrag::create();
		gesture_drag->signal_drag_begin().connect(sigc::mem_fun(*this, &sysmenu::on_drag_start));
		gesture_drag->signal_drag_update().connect(sigc::mem_fun(*this, &sysmenu::on_drag_update));
		gesture_drag->signal_drag_end().connect(sigc::mem_fun(*this, &sysmenu::on_drag_stop));

		// Set up revealer
		revealer_dock.set_child(*sysmenu_dock);
		revealer_dock.set_transition_type(Gtk::RevealerTransitionType::SLIDE_UP);
		revealer_dock.set_transition_duration(500);
		revealer_dock.set_reveal_child(true);
		revealer_search.set_reveal_child(false);

		box_layout.append(box_top);
		box_top.append(revealer_dock);
		box_top.property_orientation().set_value(Gtk::Orientation::VERTICAL);
		box_top.add_controller(gesture_drag);

		// Set window height to the dock's height
		config_main["main"]["height"] = "30";
		box_layout.set_valign(Gtk::Align::END);
	}
	else {
		gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_TOP);
	}

	// Initialize
	set_name("sysmenu");
	set_default_size(std::stoi(config_main["main"]["width"]), std::stoi(config_main["main"]["height"]));
	set_hide_on_close(true);

	box_layout.property_orientation().set_value(Gtk::Orientation::VERTICAL);
	set_child(box_layout);
	box_layout.append(scrolled_window_inner);
	scrolled_window_inner.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);
	scrolled_window_inner.set_child(box_layout_inner);
	scrolled_window_inner.set_kinetic_scrolling(false);
	box_layout_inner.set_orientation(Gtk::Orientation::VERTICAL);
	box_layout_inner.add_css_class("box_layout_inner");

	// Sadly there does not seem to be a way to detect what the default monitor is
	// Gotta assume or ask the user for their monitor of choice
	display = gdk_display_get_default();
	monitors = gdk_display_get_monitors(display);
	monitorCount = g_list_model_get_n_items(monitors);
	int monitor_idx = std::clamp(std::stoi(config_main["main"]["monitor"]), 0, monitorCount - 1);
	config_main["main"]["monitor"] = std::to_string(monitor_idx);
	monitor = GDK_MONITOR(g_list_model_get_item(monitors, monitor_idx));

	if (config_main["main"]["layer-shell"] == "true")
		gtk_layer_set_monitor(gobj(), monitor);

	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);
	max_height = geometry.height;

	uwsm_available = !Glib::find_program_in_path("uwsm").empty();

	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(sigc::mem_fun(*this, &sysmenu::on_key_press), true);
	add_controller(controller);

	if (has_searchbar) {
		entry_search.add_css_class("entry_search");
		if (is_dock) {
			box_top.append(revealer_search);
			revealer_search.add_css_class("revealer_search");
			revealer_search.set_child(entry_search);
			revealer_search.set_transition_type(Gtk::RevealerTransitionType::SLIDE_UP);
			revealer_search.set_transition_duration(500);
		}
		else {
			box_layout.prepend(entry_search);
		}

		entry_search.set_halign(Gtk::Align::CENTER);
		entry_search.set_icon_from_icon_name("system-search-symbolic", Gtk::Entry::IconPosition::PRIMARY);
		entry_search.set_placeholder_text("Search");
		entry_search.set_margin(10);
		entry_search.set_size_request(std::stoi(config_main["main"]["width"]) - 20, -1);

		entry_search.signal_changed().connect(sigc::mem_fun(*this, &sysmenu::on_search_changed));
		entry_search.signal_activate().connect([this]() {
			if (selected_child)
				run_menu_item(selected_child, false);
		});
		flowbox_itembox.set_sort_func(sigc::mem_fun(*this, &sysmenu::on_sort));
		flowbox_itembox.set_filter_func(sigc::mem_fun(*this, &sysmenu::on_filter));

		if (!is_dock)
			entry_search.grab_focus();
		else
			show();
	}

	box_layout.add_css_class("box_layout");

	if (std::stoi(config_main["main"]["items-per-row"]) != 1)
		flowbox_itembox.set_halign(Gtk::Align::CENTER);

	history_size = std::stoi(config_main["main"]["history-size"]);

	if (history_size > 0) {
		box_layout_inner.append(flowbox_recent);
		flowbox_recent.set_visible(false);
		flowbox_recent.add_css_class("flowbox_recent");

		if (std::stoi(config_main["main"]["items-per-row"]) == 1) {
			flowbox_recent.set_orientation(Gtk::Orientation::HORIZONTAL);
			flowbox_recent.set_min_children_per_line(1);
			flowbox_recent.set_max_children_per_line(1);
		}
		else {
			flowbox_recent.set_orientation(Gtk::Orientation::VERTICAL);
			flowbox_recent.set_halign(Gtk::Align::CENTER);
			flowbox_recent.set_min_children_per_line(history_size);
			flowbox_recent.set_max_children_per_line(history_size);
		}

		flowbox_recent.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
			run_menu_item(child, true);
		});
	}

	box_layout_inner.append(scrolled_window);
	scrolled_window.set_child(box_scrolled_contents);
	box_scrolled_contents.set_orientation(Gtk::Orientation::VERTICAL);
	box_scrolled_contents.append(flowbox_itembox);

	if (!has_searchbar)
		scrolled_window.set_policy(Gtk::PolicyType::EXTERNAL, Gtk::PolicyType::EXTERNAL);

	flowbox_itembox.set_valign(Gtk::Align::START);
	flowbox_itembox.set_min_children_per_line(std::stoi(config_main["main"]["items-per-row"]));
	flowbox_itembox.set_max_children_per_line(std::stoi(config_main["main"]["items-per-row"]));
	flowbox_itembox.set_vexpand(true);
	flowbox_itembox.signal_child_activated().connect([this](Gtk::FlowBoxChild* child) {
		run_menu_item(child, false);
	});

	auto load_css = [this](const std::string& path) {
		if (std::filesystem::exists(path)) {
			auto css = Gtk::CssProvider::create();
			css->load_from_path(path);
			get_style_context()->add_provider_for_display(property_display(), css, GTK_STYLE_PROVIDER_PRIORITY_USER);
		}
	};

	load_css("/usr/share/sys64/menu/style.css");
	load_css(std::string(getenv("HOME")) + "/.config/sys64/menu/style.css");

	GAppInfoMonitor* app_info_monitor = g_app_info_monitor_get();
	g_signal_connect(app_info_monitor, "changed", G_CALLBACK(+[](GAppInfoMonitor* monitor, gpointer user_data) {
		static_cast<sysmenu*>(user_data)->app_info_changed(monitor);
	}), this);

	std::thread thread_appinfo(&sysmenu::app_info_changed, this, nullptr);
	thread_appinfo.detach();

	if (config_main["main"]["start-hidden"] != "true")
		handle_signal(SIGUSR1);
}

void sysmenu::on_search_changed() {
	if (history_size > 0)
		flowbox_recent.set_visible(entry_search.get_text().empty() && !app_list_history.empty());
	matches = 0;
	selected_child = nullptr;
	flowbox_itembox.invalidate_filter();
}

bool sysmenu::on_key_press(const guint &keyval, const guint &keycode, const Gdk::ModifierType &state) {
	switch (keyval) {
		case GDK_KEY_Escape:
			handle_signal(SIGUSR2);
			break;
		case GDK_KEY_ISO_Left_Tab:
			if (has_searchbar && !is_dock)
				entry_search.grab_focus();
			break;
		case GDK_KEY_Tab: {
			auto selected_children = flowbox_itembox.get_selected_children();
			if (!selected_children.empty())
				return false;

			auto children = flowbox_itembox.get_children();
			if (selected_child == nullptr && !children.empty())
				selected_child = dynamic_cast<Gtk::FlowBoxChild*>(children[0]);

			if (selected_child) {
				flowbox_itembox.select_child(*selected_child);
				selected_child->grab_focus();
			}
			break;
		}
		case GDK_KEY_Return:
			return false;
	}
	return true;
}

bool sysmenu::on_filter(Gtk::FlowBoxChild *child) {
	auto button = dynamic_cast<launcher*>(child->get_child());
	auto text = entry_search.get_text();

	if (button->matches(text)) {
		matches++;
		if (matches == 1) {
			selected_child = child;
		}
		return true;
	}
	return false;
}

bool sysmenu::on_sort(Gtk::FlowBoxChild* a, Gtk::FlowBoxChild* b) {
	auto b1 = dynamic_cast<launcher*>(a->get_child());
	auto b2 = dynamic_cast<launcher*>(b->get_child());
	return *b2 < *b1;
}

void sysmenu::app_info_changed(GAppInfoMonitor* gappinfomonitor) {
	auto new_app_list = Gio::AppInfo::get_all();
	std::set<std::string> detected_new_apps;

	if (!app_list.empty()) {
		for (const auto& app : new_app_list) {
			if (!app)
				continue;

			auto id = app->get_id();
			if (id.empty())
				continue;

			bool found = false;
			for (const auto& old_app : app_list) {
				if (old_app && old_app->get_id() == id) {
					found = true;
					break;
				}
			}

			if (!found)
				detected_new_apps.insert(id);
		}
	}

	Glib::signal_idle().connect([this, new_app_list = std::move(new_app_list),
				      detected_new_apps = std::move(detected_new_apps)]() {
		for (const auto& id : detected_new_apps)
			new_apps.insert(id);

		app_list = new_app_list;
		flowbox_itembox.remove_all();

		if (is_dock)
			sysmenu_dock->remove_all();

		// Load applications
		for (auto app : app_list)
			load_menu_item(app);

		// Load dock items
		if (is_dock)
			sysmenu_dock->load_items(app_list);

		selected_child = nullptr;
		return false;
	});
}

void sysmenu::load_menu_item(const Glib::RefPtr<Gio::AppInfo> &app_info) {
	if (!app_info || !app_info->should_show() || !app_info->get_icon())
		return;

	auto name = app_info->get_name();
	auto exec = app_info->get_executable();

	// Skip loading empty entries
	if (name.empty() || exec.empty())
		return;

	auto id = app_info->get_id();
	bool is_new = !id.empty() && new_apps.count(id);
	items.push_back(std::make_unique<launcher>(config_main, app_info, is_new));
	flowbox_itembox.append(*items.back());
}

void sysmenu::run_menu_item(Gtk::FlowBoxChild* child, const bool &recent) {
	launcher *item = dynamic_cast<launcher*>(child->get_child());

	if (!item || !item->app_info)
		return;

	auto id = item->app_info->get_id();
	if (!id.empty() && new_apps.erase(id))
		item->clear_new_indicator();

	auto ctx = Gdk::Display::get_default()->get_app_launch_context();

	if (uwsm_available && config_main["main"]["use-uwsm"] == "true") {
		Glib::ustring cmd = "uwsm app -- " + item->app_info->get_executable();
		auto wrapped = Gio::AppInfo::create_from_commandline(cmd, "", Gio::AppInfo::CreateFlags::NONE);
		if (wrapped)
			wrapped->launch(std::vector<Glib::RefPtr<Gio::File>>{}, ctx);
		else
			item->app_info->launch(std::vector<Glib::RefPtr<Gio::File>>{}, ctx);
	} else {
		item->app_info->launch(std::vector<Glib::RefPtr<Gio::File>>{}, ctx);
	}

	handle_signal(SIGUSR2);

	if (history_size < 1 || recent)
		return;

	auto exec_path = item->app_info->get_executable();
	auto it = std::find_if(app_list_history.begin(), app_list_history.end(),
		[&exec_path](const std::shared_ptr<Gio::AppInfo>& app) {
			return app->get_executable() == exec_path;
		});

	if (it != app_list_history.end())
		return;

	if (app_list_history.size() >= history_size) {
		auto first_child = flowbox_recent.get_child_at_index(0);
		flowbox_recent.remove(*first_child);
		app_list_history.erase(app_list_history.begin());
	}

	auto recent_item = Gtk::make_managed<launcher>(config_main, item->app_info, false);
	recent_item->set_size_request(-1, -1);
	flowbox_recent.append(*recent_item);
	app_list_history.push_back(item->app_info);
	flowbox_recent.set_visible(true);
}

void sysmenu::handle_signal(const int &signum) {
	Glib::signal_idle().connect([this, signum]() {
		if (signum == SIGUSR1) {
			gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_TOP);
			flowbox_itembox.unselect_all();
			if (is_dock) {
				revealer_search.set_reveal_child(true);
				revealer_dock.set_reveal_child(false);
				box_layout.set_valign(Gtk::Align::FILL);
				box_layout.set_size_request(-1, max_height);
				gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
			}

			scrolled_window.get_vadjustment()->set_value(0);
			scrolled_window_inner.get_vadjustment()->set_value(0);

			show();

			if (has_searchbar && !is_dock)
				entry_search.grab_focus();
		}
		else if (signum == SIGUSR2) {
			if (is_dock) {
				revealer_search.set_reveal_child(false);
				revealer_dock.set_reveal_child(true);
				box_layout.set_valign(Gtk::Align::END);
				box_layout.set_size_request(-1, -1);
				gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
				gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
			}
			else {
				hide();
				gtk_layer_set_layer(gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
			}
			if (has_searchbar)
				entry_search.set_text("");
			flowbox_recent.unselect_all();
			flowbox_itembox.unselect_all();
		}
		else if (signum == SIGRTMIN) {
			if (is_dock) {
				starting_height = box_layout.get_height();
				handle_signal((box_layout.get_height() < max_height / 2) ? SIGUSR1 : SIGUSR2);
			}
			else {
				handle_signal(is_visible() ? SIGUSR2 : SIGUSR1);
			}
		}
		return false;
	});
}

void sysmenu::on_drag_start(const double &x, const double &y) {
	// For now disable swipe gestures on non touch inputs
	// since they're broken on on touch devices
	if (!gesture_drag->get_current_event()->get_pointer_emulated()) {
		gesture_drag->reset();
		return;
	}

	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);
	max_height = geometry.height;

	starting_height = box_layout.get_height();
	gtk_layer_set_anchor(gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
	box_layout.set_valign(Gtk::Align::END);
}

void sysmenu::on_drag_update(const double &x, const double &y) {
	int dock_icon_size = std::stoi(config_main["main"]["dock-icon-size"]);
	bool margin_ignore = (-y < dock_icon_size / 2);
	int height = starting_height - y;

	if (starting_height < max_height) {
		if (y > 0 || height >= max_height)
			return;
		if (margin_ignore)
			height = 0;

		gtk_layer_set_layer(gobj(), margin_ignore ? GTK_LAYER_SHELL_LAYER_TOP : GTK_LAYER_SHELL_LAYER_BOTTOM);
		revealer_dock.set_reveal_child(margin_ignore);
		revealer_search.set_reveal_child(!margin_ignore);
	} else {
		if (y < 0 || height < dock_icon_size)
			return;

		bool show_dock = (height <= dock_icon_size * 2);
		gtk_layer_set_layer(gobj(), show_dock ? GTK_LAYER_SHELL_LAYER_BOTTOM : GTK_LAYER_SHELL_LAYER_TOP);
		revealer_dock.set_reveal_child(show_dock);
		revealer_search.set_reveal_child(!show_dock);
	}

	box_layout.set_size_request(-1, height);
}

void sysmenu::on_drag_stop(const double &x, const double &y) {
	// For now disable swipe gestures on non touch inputs
	// since they're broken on on touch devices
	if (!gesture_drag->get_current_event()->get_pointer_emulated()) {
		gesture_drag->reset();
		return;
	}

	bool should_open = (box_layout.get_height() > max_height / 2);
	int target_height = should_open ? max_height : std::stoi(config_main["main"]["dock-icon-size"]);
	int current_height = box_layout.get_height();
	int duration = std::stoi(config_main["main"]["animation-duration"]);

	if (current_height == target_height) {
		handle_signal(should_open ? SIGUSR1 : SIGUSR2);
		return;
	}

	auto start_time = std::make_shared<gint64>(g_get_monotonic_time());

	Glib::signal_timeout().connect([this, current_height, target_height, duration, should_open, start_time]() {
		gint64 now = g_get_monotonic_time();
		gint64 elapsed = now - *start_time;
		float progress = std::min(1.0f, (float)elapsed / (duration * 1000.0f));

		if (progress >= 1.0f) {
			box_layout.set_size_request(-1, target_height);
			handle_signal(should_open ? SIGUSR1 : SIGUSR2);
			return false;
		}

		progress = progress * progress * (3.0f - 2.0f * progress);
		int new_height = current_height + ((target_height - current_height) * progress);
		box_layout.set_size_request(-1, new_height);

		return true;
	}, 16);
}

extern "C" {
	sysmenu *sysmenu_create(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
		return new sysmenu(cfg);
	}
	void sysmenu_signal(sysmenu *window, int signal) {
		window->handle_signal(signal);
	}
}
