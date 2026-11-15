#ifndef SCAPMS_GUI_QUEUEPANEL_H
#define SCAPMS_GUI_QUEUEPANEL_H

#include <QWidget>

#include "../controller/ClinicManager.h"

class QTableWidget;
class QComboBox;
class QPushButton;
class QLabel;

namespace scapms {
namespace gui {

/**
 * QueuePanel - the std::queue backed waiting list.
 * First come first served: the table shows queue order and live
 * waiting times (refreshed by the MainWindow timer via refresh()).
 */
class QueuePanel : public QWidget {
    Q_OBJECT
public:
    explicit QueuePanel(controller::ClinicManager& manager,
                        QWidget* parent = nullptr);

    void refresh();
    void refreshPatients();

signals:
    void dataChanged();

private slots:
    void onEnqueueClicked();
    void onCallNextClicked();

private:
    void buildUi();

    controller::ClinicManager& manager_;

    QComboBox*    patientCombo_ = nullptr;
    QPushButton*  enqueueButton_ = nullptr;
    QPushButton*  callNextButton_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel*       frontLabel_ = nullptr;
};

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_QUEUEPANEL_H
