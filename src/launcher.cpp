#include "launcher.hpp"
#include <QEvent>
#include <QKeyEvent>
#include <QProcess>

launcher::launcher(const QString& name, const QIcon& icon, const QString& comment, 
				   int icon_size, int name_limit, bool vertical,
				   ClickCallback on_click, QWidget* parent)
	: QWidget(parent), name(name), exec(""), click_callback(std::move(on_click)) {

	setObjectName("application");
	QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
	if (vertical)
		layout->setDirection(QBoxLayout::TopToBottom);

	icon_label = new QLabel();
	QPixmap pixmap = icon.pixmap(icon_size, icon_size);
	icon_label->setPixmap(pixmap);
	icon_label->setAlignment(Qt::AlignCenter);
	icon_label->setMinimumSize(icon_size, icon_size);
	
	QString display_name = name;
	if (display_name.length() > name_limit) {
		display_name = display_name.left(name_limit - 3) + "...";
	}
	text_label = new QLabel(display_name);
	if (vertical)
		text_label->setAlignment(Qt::AlignCenter);
	else
		text_label->setAlignment(Qt::AlignLeft);
	text_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	
	layout->addWidget(icon_label);
	layout->addWidget(text_label);
	
	if (!comment.isEmpty()) {
		setToolTip(comment);
	}
	
	icon_label->installEventFilter(this);
	text_label->installEventFilter(this);
	this->installEventFilter(this);
}

void launcher::set_click_callback(ClickCallback callback) {
	click_callback = std::move(callback);
}

bool launcher::eventFilter(QObject* obj, QEvent* event) {
	if (event->type() == QEvent::MouseButtonRelease) {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::LeftButton) {
			if (obj == icon_label || obj == text_label || obj == this) {
				handle_click("");
				return true;
			}
		}
	}
	return QWidget::eventFilter(obj, event);
}

void launcher::handle_click(const QString& terminal_cmd) {
	if (exec.isEmpty()) return;

	QString command = exec;
	command = command.replace("%f", "").replace("%F", "")
					.replace("%u", "").replace("%U", "")
					.replace("%i", "").replace("%c", "")
					.trimmed();

	if (!terminal_cmd.isEmpty() && (exec.contains("Terminal=true", Qt::CaseInsensitive))) {
		QStringList terminalArgs = terminal_cmd.split(' ');
		QString program = terminalArgs.takeFirst();
		terminalArgs << command;
		QProcess::startDetached(program, terminalArgs);
	} else {
		QProcess::startDetached(command);
	}

	if (click_callback) {
		click_callback();
	}
}

void launcher::trigger_click(const QString& terminal_cmd) {
	handle_click(terminal_cmd);
}