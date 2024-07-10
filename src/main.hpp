#pragma once
#include "window.hpp"

void handle_signal(int);
inline sysmenu* win;

typedef sysmenu* (*sysmenu_create_func)();
inline sysmenu_create_func sysmenu_create_ptr;
