#pragma once
#include "config.hpp"
#include "window.hpp"

sysmenu* win;

typedef sysmenu* (*sysmenu_create_func)(const std::map<std::string, std::map<std::string, std::string>>&);
sysmenu_create_func sysmenu_create_ptr;

typedef void (*sysmenu_handle_signal_func)(sysmenu*, int);
sysmenu_handle_signal_func sysmenu_handle_signal_ptr;
