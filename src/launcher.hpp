#include <gtkmm.h>
#include <giomm/desktopappinfo.h>

class launcher : public Gtk::Button {
	public:
		launcher(AppInfo app);
		AppInfo app_info;

		bool matches(Glib::ustring text);
		bool operator < (const launcher& other);

	private:
		Gtk::Box box_launcher;
		Gtk::Image image_program;
		Gtk::Label label_program;
		void on_click();
};