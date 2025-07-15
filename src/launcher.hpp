#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class launcher : public QWidget {
	public:
		using ClickCallback = std::function<void()>;
		
		launcher(const QString& name, const QIcon& icon, const QString& comment, 
				int icon_size, int name_limit, bool vertical, 
				ClickCallback on_click = nullptr, QWidget* parent = nullptr);

		QString name;
		QString exec;
		void trigger_click(const QString& terminal_cmd);
		void set_click_callback(ClickCallback callback);

	protected:
		bool eventFilter(QObject* obj, QEvent* event) override;

	private:

		QLabel* icon_label;
		QLabel* text_label;
		ClickCallback click_callback;

		void handle_click(const QString& terminal_cmd);
};