#pragma once
#define QT_NO_SIGNALS_SLOTS_KEYWORDS
#include "launcher.hpp"

#include <string>
#include <map>
#include <vector>
#include <future>
#include <functional>
#include <QLineEdit>
#include <QScrollArea>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QAction>
#include <QWindow>

class sysmenu : public QWidget {
	public:
		sysmenu(const std::map<std::string, std::map<std::string, std::string>>& cfg);
		void handle_signal(const int& signum);

	private:
		std::map<std::string, std::map<std::string, std::string>> config;
		QLineEdit* search_entry;
		QScrollArea* scroll_area;
		QWidget* scroll_content;
		QLayout* scroll_layout;
		QFileSystemWatcher* watcher;
		QTimer* refresh_timer;
		QAction* search_action;
		std::vector<launcher*> all_items;
		std::vector<launcher*> visible_items;
		std::future<void> load_future;
		QWindow* handle;
		QString terminal_cmd;

		unsigned short icon_size;
		unsigned short app_margins;
		unsigned short name_length;
		short monitor;
		bool name_under_icon;
		bool scroll_bars;
		bool searchbar_enabled;
		unsigned short items_per_row;

		void setup_window();
		void setup_layer_shell();
		void load_applications();
		void filter_items(const QString& text);
		void setup_watcher();
		QString get_desktop_file_info(const QString& path, const QString& key);
		bool should_show_application(const QString& desktop_file_path);
		bool eventFilter(QObject* obj, QEvent* event) override;
		void launch_app(const QString& exec);
		void load_qss(const std::string& filePath);
};