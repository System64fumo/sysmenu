#pragma once

#include "window.hpp"

#include <giomm/desktopappinfo.h>
#include <gtkmm.h>

using AppInfo = Glib::RefPtr<Gio::AppInfo>;
inline Glib::RefPtr<Gtk::Application> app;
void handle_signal(int);
inline sysmenu* win;
