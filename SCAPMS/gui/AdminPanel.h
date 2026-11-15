#ifndef SCAPMS_GUI_ADMINPANEL_H
#define SCAPMS_GUI_ADMINPANEL_H

#include <QWidget>

#include "../controller/ClinicManager.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;

namespace scapms {
namespace gui {

/**
 * AdminPanel - administrator tools:
 *  - Undo Last Administrative Operation (std::stack),
 *  - live dashboard statistics (refreshed by the MainWindow timer),
 *  - frequently accessed patient records (fixed-size array cache),
 *  - clinic report built through polymorphic processing,
 *  - today's appointment reminder list.
 */
class AdminPanel : public QWidget {
    Q_OBJECT
public:
    explicit AdminPanel(controller::ClinicManager& manager,
                        QWidget* parent = nullptr);

    void refresh();      // statistics + undo state (timer calls this)
    void updateReminders(const std::string& today); // date "YYYY-MM-DD"

signals:
    void dataChanged();

public slots:
    void onUndoClicked();

private:
    void buildUi();

    controller::ClinicManager& manager_;

    QLabel*      statsLabel_ = nullptr;
    QLabel*      undoHintLabel_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QTableWidget* cacheTable_ = nullptr;
    QPlainTextEdit* reportView_ = nullptr;
    QPushButton* reportButton_ = nullptr;
    QLabel*      reminderLabel_ = nullptr;
};

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_ADMINPANEL_H
