#include <gtkmm.h>
#include <giomm/desktopappinfo.h>

using AppInfo = Glib::RefPtr<Gio::AppInfo>;
inline Glib::RefPtr<Gtk::Application> app;
void handle_signal(int);

class sysmenu : public Gtk::Window {

	int matches = 0;
	Glib::ustring match = "";

	public:
		Gtk::Entry entry_search;
		Gtk::FlowBox flowbox_itembox;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::Box box_layout;
		Gtk::Box box_grabber;
		Gtk::CenterBox centerbox_top;

		sysmenu();
		void load_menu_item(AppInfo app_info);

	private:
		Glib::RefPtr<Gtk::GestureDrag> gesture_drag;


		void InitLayout();
		bool on_escape_key_press(guint keyval, guint keycode, Gdk::ModifierType state);

		bool on_sort(Gtk::FlowBoxChild*, Gtk::FlowBoxChild*);
		bool on_filter(Gtk::FlowBoxChild* child);
		void on_search_changed();
		void on_search_done();

		void on_drag_start(int x, int y);
		void on_drag_update(int x, int y);
		void on_drag_stop(int x, int y);
};
