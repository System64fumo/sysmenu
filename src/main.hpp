#pragma once
#include "config.hpp"
#include "window.hpp"

void handle_signal(int);
inline sysmenu* win;

typedef sysmenu* (*sysmenu_create_func)(const config &cfg);
typedef void (*sysmenu_handle_signal_func)(sysmenu*, int);
inline sysmenu_create_func sysmenu_create_ptr;
inline sysmenu_handle_signal_func sysmenu_handle_signal_ptr;
