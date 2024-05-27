#include <gtkmm.h>

class css_loader : public Glib::RefPtr<Gtk::StyleProvider> {
	public:
		css_loader(std::string path, Gtk::Window *window);
};
