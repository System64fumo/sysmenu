#pragma once

#include "window.hpp"

#include <gtkmm/application.h>

inline Glib::RefPtr<Gtk::Application> app;
void handle_signal(int);
inline sysmenu* win;
