#ifndef SCAPMS_GUI_PATIENTPANEL_H
#define SCAPMS_GUI_PATIENTPANEL_H

#include <QWidget>

#include "../controller/ClinicManager.h"

class QTableWidget;
class QLineEdit;
class QSpinBox;
class QComboBox;
class QStackedWidget;
class QPushButton;
class QLabel;

namespace scapms {
namespace gui {

/**
 * PatientPanel - CRUD tab for patient records.
 *
 * Layout techniques used (assignment requirement 8):
 *  - QHBoxLayout splits the tab into table (left) / form (right),
 *  - QVBoxLayout stacks the sub-groups vertically,
 *  - QFormLayout arranges the labelled input fields,
 *  - QStackedWidget switches the type-specific extra fields at runtime
 *    (Dynamic Components, requirement 9b).
 */
class PatientPanel : public QWidget {
    Q_OBJECT
public:
    explicit PatientPanel(controller::ClinicManager& manager,
                          QWidget* parent = nullptr);

    void refresh(); // rebuild the table from the database

signals:
    void dataChanged();

private slots:
    void onAddClicked();
    void onUpdateClicked();
    void onDeleteClicked();
    void onTypeChanged(int index);
    void onSelectionChanged();

private:
    void buildUi();
    void loadFormFromPatient(const model::PatientRecord* patient);
    void clearForm();
    // Collects extra1/extra2 for the currently selected type from the
    // stacked pages; throws std::invalid_argument on bad severity input.
    void collectExtras(std::string& extra1, std::string& extra2) const;

    controller::ClinicManager& manager_;

    QTableWidget*   table_       = nullptr;
    QLineEdit*      idLabel_     = nullptr; // read-only display of selected ID
    QComboBox*      typeCombo_   = nullptr;
    QLineEdit*      nameEdit_    = nullptr;
    QSpinBox*       ageSpin_     = nullptr;
    QLineEdit*      contactEdit_ = nullptr;

    // Type specific pages inside the stacked widget.
    QStackedWidget* extrasStack_ = nullptr;
    QLineEdit*      reasonEdit_    = nullptr; // GENERAL
    QComboBox*      severityCombo_ = nullptr; // EMERGENCY
    QLineEdit*      descEdit_      = nullptr; // EMERGENCY
    QLineEdit*      lastVisitEdit_ = nullptr; // FOLLOWUP
    QLineEdit*      noteEdit_      = nullptr; // FOLLOWUP

    QPushButton* addButton_    = nullptr;
    QPushButton* updateButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QLabel*      hintLabel_    = nullptr;

    std::string selectedPatientId_;
};

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_PATIENTPANEL_H
