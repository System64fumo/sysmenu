#include <gtkmm.h>
#include <giomm/desktopappinfo.h>

using AppInfo = Glib::RefPtr<Gio::AppInfo>;

class launcher : public Gtk::Box {
	public:
		launcher(AppInfo app);
		AppInfo app_info;

		bool matches(Glib::ustring text);
		bool operator < (const launcher& other);

	private:
		Gtk::Button button_launcher;
		Gtk::Box box_launcher;
		Gtk::Image image_program;
		Gtk::Label label_program;
		void on_click();
};

class sysmenu : public Gtk::Window {

	int32_t count_matches = 0;
	Glib::ustring first_match = "";

	public:
		Gtk::Entry entry_search;
		sysmenu();

	private:
		Gtk::Box box_layout;
		Gtk::CenterBox centerbox_top;
		Gtk::ScrolledWindow scrolled_window;
		Gtk::FlowBox flowbox_itembox;

		void InitLayout();
		bool on_escape_key_press(guint keyval, guint keycode, Gdk::ModifierType state);
		void load_menu_item(AppInfo app_info);

		bool on_sort(Gtk::FlowBoxChild*, Gtk::FlowBoxChild*);
		bool on_filter(Gtk::FlowBoxChild* child);
		void on_search_changed();
		void on_search_done();
};
