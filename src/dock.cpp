#include "dock.hpp"

dock::dock() {
	get_style_context()->add_class("dock");
	set_size_request(-1, 100);
}
