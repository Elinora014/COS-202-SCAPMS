#ifndef SCAPMS_GUI_APPOINTMENTPANEL_H
#define SCAPMS_GUI_APPOINTMENTPANEL_H

#include <QWidget>

#include "../controller/AppointmentController.h"

class QTableWidget;
class QComboBox;
class QLineEdit;
class QDateEdit;
class QTimeEdit;
class QPushButton;

namespace scapms {
namespace gui {

/**
 * AppointmentPanel - schedule / cancel / complete / view appointments.
 * Uses QGridLayout for the entry form and a QHBoxLayout split against
 * the appointments table.
 */
class AppointmentPanel : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentPanel(controller::ClinicManager& manager,
                              controller::AppointmentController& controller,
                              QWidget* parent = nullptr);

    void refresh();          // rebuild the appointment table
    void refreshPatients();  // rebuild the patient dropdown only

signals:
    void dataChanged();

private slots:
    void onScheduleClicked();
    void onCancelClicked();
    void onCompleteClicked();

private:
    void buildUi();
    std::string selectedAppointmentId() const;

    controller::ClinicManager& manager_;
    controller::AppointmentController& controller_;

    QComboBox*   patientCombo_ = nullptr;
    QDateEdit*   dateEdit_     = nullptr;
    QTimeEdit*   timeEdit_     = nullptr;
    QLineEdit*   serviceEdit_  = nullptr;
    QTableWidget* table_       = nullptr;
    QPushButton* scheduleButton_ = nullptr;
    QPushButton* cancelButton_    = nullptr;
    QPushButton* completeButton_  = nullptr;
};

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_APPOINTMENTPANEL_H
