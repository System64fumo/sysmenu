#include "window.hpp"
#include <signal.h>
#include <QPainter>
#include <QPixmap>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QKeyEvent>
#include <QToolTip>
#include <QScrollBar>
#include <QScroller>
#include <LayerShellQt/Shell>

sysmenu::sysmenu(const std::map<std::string, std::map<std::string, std::string>>& cfg) 
	: config(cfg), handle(nullptr) {

	icon_size = std::stoi(config["main"]["icon-size"]);
	app_margins = std::stoi(config["main"]["app-margins"]);
	name_length = std::stoi(config["main"]["name-length"]);
	scroll_bars = config["main"]["scroll-bars"] == "true";
	items_per_row = std::stoi(config["main"]["items-per-row"]);
	terminal_cmd = QString::fromStdString(config["main"]["terminal-cmd"]); // TODO: Fix this
	name_under_icon = config["main"]["name-under-icon"] == "true";
	searchbar_enabled = config["main"]["searchbar"] == "true";
	monitor = std::stoi(config["main"]["monitor"]);
	dock_items = config["main"]["dock-items"];
	dock_icon_size = std::stoi(config["main"]["dock-icon-size"]);
	dock_enabled = dock_items != "";

	setup_window();
	setup_layer_shell();
	setup_watcher();
	max_height = handle->screen()->geometry().height();
	
	load_future = std::async(std::launch::async, [this]() {
		load_applications();
	});

	if (dock_enabled)
		show();

	if (config["main"]["start-hidden"] == "true")
		handle_signal(SIGUSR2);
	else
		handle_signal(SIGUSR1);
}

void sysmenu::setup_layer_shell() {
	createWinId();
	handle = windowHandle();

	layer_shell = LayerShellQt::Window::get(handle);

	QString anchors = QString::fromStdString(config["main"]["anchors"]);
	LayerShellQt::Window::Anchors anchor_flags;
	if (anchors.contains("top") && !dock_enabled) anchor_flags |= LayerShellQt::Window::AnchorTop;
	if (anchors.contains("bottom")) anchor_flags |= LayerShellQt::Window::AnchorBottom;
	if (anchors.contains("left")) anchor_flags |= LayerShellQt::Window::AnchorLeft;
	if (anchors.contains("right")) anchor_flags |= LayerShellQt::Window::AnchorRight;

	layer_shell->setAnchors(anchor_flags);

	layer_shell->setExclusiveZone(-1);
	layer_shell->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
	if (config["main"]["start-hidden"] == "true")
		layer_shell->setLayer(LayerShellQt::Window::LayerBottom);
	else
		layer_shell->setLayer(LayerShellQt::Window::LayerTop);

	if (monitor != -1) {
		auto screens = QGuiApplication::screens();
		if (monitor >= 0 && monitor < screens.size())
			layer_shell->setScreenConfiguration(static_cast<LayerShellQt::Window::ScreenConfiguration>(monitor));
	}
	else {
		layer_shell->setScreenConfiguration(LayerShellQt::Window::ScreenFromCompositor);
	}

	layer_shell->setScope("sysmenu");
}

void sysmenu::setup_window() {
	setWindowTitle("sysmenu");
	setObjectName("sysmenu");
	resize(std::stoi(config["main"]["width"]), std::stoi(config["main"]["height"]));

	const std::string& style_path = "/usr/share/sys64/menu/style.qss";
	const std::string& style_path_usr = std::string(getenv("HOME")) + "/.config/sys64/menu/style.qss";

	if (std::filesystem::exists(style_path))
		load_qss(style_path);

	if (std::filesystem::exists(style_path_usr))
		load_qss(style_path_usr);

	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_AcceptTouchEvents);

	QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->setAlignment(Qt::AlignCenter);

	QWidget* container = new QWidget();
	container->setObjectName("container");
	QVBoxLayout* main_layout = new QVBoxLayout(container);
	main_layout->setContentsMargins(10, 10, 10, 10);
	main_layout->setSpacing(10);
	main_layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

	if (searchbar_enabled) {
		search_entry = new QLineEdit();
		search_entry->setFixedWidth(std::stoi(config["main"]["width"]));
		search_entry->setPlaceholderText("Search");
		search_entry->setObjectName("search_entry");

		search_action = new QAction(search_entry);
		search_action->setIcon(QIcon::fromTheme("edit-find-symbolic"));
		search_entry->addAction(search_action, QLineEdit::LeadingPosition);

		QObject::connect(search_entry, &QLineEdit::textChanged, [this](const QString& text) {
			this->filter_items(text);
		});

		search_entry->installEventFilter(this);
		main_layout->addWidget(search_entry, 1, {Qt::AlignHCenter});
	}
	
	QWidget* scroll_container = new QWidget();
	scroll_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QHBoxLayout* scroll_container_layout = new QHBoxLayout(scroll_container);
	scroll_container_layout->setContentsMargins(0, 0, 0, 0);
	scroll_container_layout->setSpacing(0);
	scroll_container_layout->setAlignment(Qt::AlignHCenter);
	
	scroll_area = new QScrollArea();
	scroll_area->setHorizontalScrollBarPolicy(scroll_bars ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	scroll_area->setVerticalScrollBarPolicy(scroll_bars ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	
	QScroller::grabGesture(scroll_area->viewport(), QScroller::LeftMouseButtonGesture);
	
	scroll_content = new QWidget();
	
	if (items_per_row > 1) {
		scroll_layout = new QGridLayout(scroll_content);
		scroll_area->setAlignment(Qt::AlignCenter);
		scroll_content->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	}
	else {
		scroll_layout = new QVBoxLayout(scroll_content);
	}
	scroll_layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	scroll_layout->setContentsMargins(app_margins, app_margins, app_margins, app_margins);
	scroll_layout->setSpacing(app_margins);
	
	scroll_area->setWidget(scroll_content);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	
	scroll_container_layout->addWidget(scroll_area);
	main_layout->addWidget(scroll_container);
	layout->addWidget(container);
}

bool sysmenu::eventFilter(QObject* obj, QEvent* event) {
	if (obj == search_entry && event->type() == QEvent::KeyPress) {
		QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
		if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
			if (!visible_items.empty()) {
				visible_items.front()->trigger_click(terminal_cmd);
				handle_signal(SIGUSR2);
			}
			return true;
		}
		else if (key_event->key() == Qt::Key_Escape) {
			handle_signal(SIGUSR2);
			return true;
		}
	}
	return QWidget::eventFilter(obj, event);
}

bool sysmenu::event(QEvent *event) {
	switch (event->type()) {
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd:
		case QEvent::TouchCancel: {
			QTouchEvent* touch_event = static_cast<QTouchEvent*>(event);
			QPoint touch_pos = touch_event->points()[0].position().toPoint();
			
			if (scroll_content && scroll_content->geometry().contains(touch_pos)) {
				return QWidget::event(event);
			}
			
			handle_touch_event(touch_event);
			return true;
		}
		default:
			return QWidget::event(event);
	}
}

void sysmenu::handle_touch_event(QTouchEvent *event) {
	auto touch_point = event->points()[0];
	int desired_height = 0;
	LayerShellQt::Window::Anchors anchor_flags;

	switch (event->type()) {
		case QEvent::TouchBegin:
			printf("Touch start\n");
			max_height = handle->screen()->geometry().height();
			starting_height = height();

			anchor_flags |= LayerShellQt::Window::AnchorBottom;
			anchor_flags |= LayerShellQt::Window::AnchorLeft;
			anchor_flags |= LayerShellQt::Window::AnchorRight;
			layer_shell->setAnchors(anchor_flags);

			break;
		case QEvent::TouchUpdate:
			printf("Touch update\n");
			desired_height = starting_height - touch_point.position().y();

			if (desired_height > dock_icon_size + 20) {
				scroll_content->show();
				if (searchbar_enabled)
					search_entry->show();
				setFixedHeight(std::max(desired_height, 0));
			}
			else {
				scroll_content->hide();
				if (searchbar_enabled)
					search_entry->hide();
				setFixedHeight(dock_icon_size + 20);
			}
			break;
		case QEvent::TouchCancel:
		case QEvent::TouchEnd:
			printf("Touch end\n");

			// Top position
			if (height() > max_height / 2)
				handle_signal(SIGUSR1);

			// Bottom Position
			else
				handle_signal(SIGUSR2);

			break;
		default:
			return;
	}
	printf("Pos: %f, %f\n", touch_point.position().x(), touch_point.position().y());
}

QString sysmenu::get_desktop_file_info(const QString& path, const QString& key) {
	static QHash<QString, QString> cache;
	QString cache_key = path + ":" + key;
	
	if (cache.contains(cache_key)) {
		return cache[cache_key];
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return "";
	}

	QTextStream in(&file);
	bool in_desktop_entry = false;
	QString result;

	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.startsWith("[") && line.endsWith("]")) {
			in_desktop_entry = (line == "[Desktop Entry]");
			continue;
		}

		if (in_desktop_entry && line.startsWith(key + "=")) {
			result = line.mid(key.length() + 1).trimmed();
			break;
		}
	}

	cache[cache_key] = result;
	return result;
}

void sysmenu::load_applications() {
	QStringList app_dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
	std::vector<std::pair<QString, QIcon>> app_data;
	std::unordered_set<QString> seen_apps;

	for (const QString& dir_path : app_dirs) {
		QDir dir(dir_path);
		if (!dir.exists()) continue;

		for (const QFileInfo& file_info : dir.entryInfoList({"*.desktop"}, QDir::Files)) {
			QString file_path = file_info.absoluteFilePath();
			
			if (!should_show_application(file_path)) {
				continue;
			}

			QString name = get_desktop_file_info(file_path, "Name");
			QString exec = get_desktop_file_info(file_path, "Exec");
			QString icon_name = get_desktop_file_info(file_path, "Icon");
			QString comment = get_desktop_file_info(file_path, "Comment");

			if (name.isEmpty() || exec.isEmpty()) {
				continue;
			}

			if (seen_apps.find(name) != seen_apps.end()) {
				continue;
			}
			seen_apps.insert(name);

			QIcon icon = icon_name.isEmpty() ? 
				QIcon::fromTheme("application-x-executable") : 
				QIcon::fromTheme(icon_name, QIcon::fromTheme("application-x-executable"));

			app_data.emplace_back(name + "\t" + exec + "\t" + comment, icon);
		}
	}

	std::sort(app_data.begin(), app_data.end(), 
		[](const auto& a, const auto& b) {
			return QString::compare(a.first.split('\t')[0], b.first.split('\t')[0], Qt::CaseInsensitive) < 0;
		});

	QMetaObject::invokeMethod(this, [this, app_data = std::move(app_data)]() {
		int row = 0, col = 0;
		for (const auto& [data, icon] : app_data) {
			auto parts = data.split('\t');
			launcher* item = new launcher(
				parts[0], icon, parts[2], icon_size, name_length, name_under_icon,
				[this]() { handle_signal(SIGUSR2); },
				scroll_content
			);
			item->exec = parts[1];
			
			if (items_per_row > 1) {
				qobject_cast<QGridLayout*>(scroll_layout)->addWidget(item, row, col);
				col++;
				if (col >= items_per_row) {
					col = 0;
					row++;
				}
			} else {
				qobject_cast<QVBoxLayout*>(scroll_layout)->addWidget(item);
			}
			
			all_items.push_back(item);
			visible_items.push_back(item);
		}
	});
}

bool sysmenu::should_show_application(const QString& desktop_file_path) {
	QString only_show_in = get_desktop_file_info(desktop_file_path, "OnlyShowIn");
	QString not_show_in = get_desktop_file_info(desktop_file_path, "NotShowIn");
	
	if (!only_show_in.isEmpty())
		return false;

	const bool no_display = get_desktop_file_info(desktop_file_path, "NoDisplay") == "true";
	const bool hidden = get_desktop_file_info(desktop_file_path, "Hidden") == "true";
	if (no_display || hidden)
		return false;

	return true;
}

void sysmenu::filter_items(const QString& text) {
	for (auto item : visible_items) {
		scroll_layout->removeWidget(item);
		item->hide();
	}
	visible_items.clear();
	
	int row = 0, col = 0;
	for (auto item : all_items) {
		if (text.isEmpty() || item->name.contains(text, Qt::CaseInsensitive)) {
			if (items_per_row > 1) {
				qobject_cast<QGridLayout*>(scroll_layout)->addWidget(item, row, col);
				col++;
				if (col >= items_per_row) {
					col = 0;
					row++;
				}
			}
			else {
				qobject_cast<QVBoxLayout*>(scroll_layout)->addWidget(item);
			}
			item->show();
			visible_items.push_back(item);
		}
	}
}

void sysmenu::setup_watcher() {
	watcher = new QFileSystemWatcher(this);
	refresh_timer = new QTimer(this);
	refresh_timer->setSingleShot(true);
	refresh_timer->setInterval(500);

	QStringList app_dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
	for (const QString& dir_path : app_dirs) {
		if (QDir(dir_path).exists())
			watcher->addPath(dir_path);
	}

	connect(watcher, &QFileSystemWatcher::directoryChanged, [this]() {
		refresh_timer->start();
	});
	connect(refresh_timer, &QTimer::timeout, [this]() {
		this->load_applications();
	});
}

void sysmenu::handle_signal(const int& signum) {
	if (signum == SIGUSR1) {
		printf("Show\n");

		if (dock_enabled) {
			LayerShellQt::Window::Anchors anchor_flags;
			anchor_flags |= LayerShellQt::Window::AnchorTop;
			anchor_flags |= LayerShellQt::Window::AnchorBottom;
			anchor_flags |= LayerShellQt::Window::AnchorLeft;
			anchor_flags |= LayerShellQt::Window::AnchorRight;
			layer_shell->setAnchors(anchor_flags);
			scroll_content->show();
			if (searchbar_enabled)
				search_entry->show();
		}
		else {
			show();
		}

		if (searchbar_enabled)
			search_entry->setFocus();

		layer_shell->setLayer(LayerShellQt::Window::LayerTop);
	}
	else if (signum == SIGUSR2) {
		printf("Hide\n");
		search_entry->clear();
		scroll_area->verticalScrollBar()->setValue(0);
		scroll_area->horizontalScrollBar()->setValue(0);
		filter_items("");

		if (dock_enabled) {
			LayerShellQt::Window::Anchors anchor_flags;
			anchor_flags |= LayerShellQt::Window::AnchorBottom;
			anchor_flags |= LayerShellQt::Window::AnchorLeft;
			anchor_flags |= LayerShellQt::Window::AnchorRight;
			layer_shell->setAnchors(anchor_flags);
			scroll_content->hide();
			if (searchbar_enabled)
				search_entry->hide();
			setFixedHeight(dock_icon_size + 20); // TODO: This could be better..
		}
		else {
			hide();
		}
		layer_shell->setLayer(LayerShellQt::Window::LayerBottom);
	}
	else if (signum == SIGRTMIN) {
		if (dock_enabled) {
			if (height() > max_height / 2)
				handle_signal(SIGUSR2);
			else
				handle_signal(SIGUSR1);
		}
		else {
			if (isVisible())
				handle_signal(SIGUSR2);
			else
				handle_signal(SIGUSR1);
		}
	}
}

void sysmenu::load_qss(const std::string& filePath) {
	QFile styleFile(QString::fromStdString(filePath));
	if (styleFile.open(QFile::ReadOnly)) {
		QString styleSheet = QLatin1String(styleFile.readAll());
		setStyleSheet(styleSheet);
		styleFile.close();
	}
	else {
		std::fprintf(stderr, "Failed to load stylesheet: %s\n", filePath.c_str());
	}
}

extern "C" {
	sysmenu *sysmenu_create(const std::map<std::string, std::map<std::string, std::string>>& cfg) {
		return new sysmenu(cfg);
	}

	void sysmenu_signal(sysmenu *window, int signal) {
		window->handle_signal(signal);
	}
}