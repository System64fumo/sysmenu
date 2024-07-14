#pragma once
#include "config.hpp"
#include "window.hpp"

config_menu config_main;
sysmenu* win;

typedef sysmenu* (*sysmenu_create_func)(const config_menu &cfg);
typedef void (*sysmenu_handle_signal_func)(sysmenu*, int);
sysmenu_create_func sysmenu_create_ptr;
sysmenu_handle_signal_func sysmenu_handle_signal_ptr;
