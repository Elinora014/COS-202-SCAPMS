#include "AppointmentPanel.h"
#include "GuiHelpers.h"

#include "../utils/Exceptions.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>
#include <QDate>
#include <QTime>

namespace scapms {
namespace gui {

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager,
                                   controller::AppointmentController& controller,
                                   QWidget* parent)
    : QWidget(parent), manager_(manager), controller_(controller)
{
    buildUi();
    refreshPatients();
    refresh();
}

void AppointmentPanel::buildUi()
{
    // ---- entry form (QGridLayout: two-dimensional label/field grid) ----
    patientCombo_ = new QComboBox(this);
    patientCombo_->setToolTip("Only registered patients can get appointments.");

    dateEdit_ = new QDateEdit(QDate::currentDate(), this);
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    dateEdit_->setToolTip("Appointment date (calendar picker).");

    timeEdit_ = new QTimeEdit(QTime(9, 0), this);
    timeEdit_->setDisplayFormat("HH:mm");
    timeEdit_->setToolTip("Appointment time in 24-hour format.");

    serviceEdit_ = new QLineEdit(this);
    serviceEdit_->setPlaceholderText("e.g. General consultation");

    auto* form = new QGridLayout;
    form->addWidget(new QLabel("Patient:", this), 0, 0);
    form->addWidget(patientCombo_, 0, 1, 1, 3);
    form->addWidget(new QLabel("Date:", this), 1, 0);
    form->addWidget(dateEdit_, 1, 1);
    form->addWidget(new QLabel("Time:", this), 1, 2);
    form->addWidget(timeEdit_, 1, 3);
    form->addWidget(new QLabel("Service:", this), 2, 0);
    form->addWidget(serviceEdit_, 2, 1, 1, 3);

    auto* formGroup = new QGroupBox("Schedule a New Appointment", this);
    auto* formGroupLayout = new QVBoxLayout(formGroup);
    formGroupLayout->addLayout(form);

    // ---- table ----
    table_ = new QTableWidget(this);
    table_->setColumnCount(7);
    QStringList headers;
    headers << "Appt ID" << "Patient ID" << "Patient" << "Date"
            << "Time" << "Service" << "Status";
    table_->setHorizontalHeaderLabels(headers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);

    // ---- action buttons ----
    scheduleButton_ = new QPushButton("Schedule Appointment", this);
    scheduleButton_->setToolTip("Create an appointment for the patient "
                                "selected above (double booking is rejected).");
    cancelButton_ = new QPushButton("Cancel Selected", this);
    cancelButton_->setToolTip("Cancel the appointment selected in the table "
                              "(Emergency patients with high severity refuse).");
    completeButton_ = new QPushButton("Mark Completed", this);
    completeButton_->setToolTip("Record that the consultation took place.");

    connect(scheduleButton_, &QPushButton::clicked,
            this, &AppointmentPanel::onScheduleClicked);
    connect(cancelButton_, &QPushButton::clicked,
            this, &AppointmentPanel::onCancelClicked);
    connect(completeButton_, &QPushButton::clicked,
            this, &AppointmentPanel::onCompleteClicked);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(scheduleButton_);
    buttonRow->addWidget(cancelButton_);
    buttonRow->addWidget(completeButton_);
    buttonRow->addStretch();

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->addWidget(formGroup);
    outer->addWidget(table_, 1);
    outer->addLayout(buttonRow);
}

std::string AppointmentPanel::selectedAppointmentId() const
{
    const int row = table_->currentRow();
    if (row < 0)
        return "";
    return toStd(table_->item(row, 0)->text());
}

void AppointmentPanel::refreshPatients()
{
    const QString previous = patientCombo_->currentText();
    patientCombo_->blockSignals(true);
    patientCombo_->clear();
    for (const auto& p : manager_.database().patients()) {
        // "P0001 - Jane Doe"
        patientCombo_->addItem(toQt(p->getPatientId() + " - " + p->getName()));
    }
    const int idx = previous.isEmpty() ? -1 : patientCombo_->findText(previous);
    if (idx >= 0)
        patientCombo_->setCurrentIndex(idx);
    patientCombo_->blockSignals(false);
}

void AppointmentPanel::refresh()
{
    refreshPatients();

    table_->setRowCount(0);
    for (const auto& a : manager_.database().appointments()) {
        const int row = table_->rowCount();
        table_->insertRow(row);

        std::string patientName = "(unknown)";
        if (auto* p = manager_.database().findPatientRaw(a.getPatientId()))
            patientName = p->getName();

        table_->setItem(row, 0, plainItem(toQt(a.getId())));
        table_->setItem(row, 1, plainItem(toQt(a.getPatientId())));
        table_->setItem(row, 2, plainItem(toQt(patientName)));
        table_->setItem(row, 3, plainItem(toQt(a.getDate())));
        table_->setItem(row, 4, plainItem(toQt(a.getTime())));
        table_->setItem(row, 5, plainItem(toQt(a.getService())));
        table_->setItem(row, 6,
                        plainItem(toQt(model::Appointment::statusText(a.getStatus()))));

        // Custom presentation by appointment status (requirement 9a).
        paintRow(table_, row, colorForAppointmentStatus(a.getStatus()));
    }
    table_->resizeColumnsToContents();
}

void AppointmentPanel::onScheduleClicked()
{
    const QString shown = patientCombo_->currentText();
    if (shown.isEmpty()) {
        QMessageBox::warning(this, "No Patient",
                             "Register a patient before scheduling appointments.");
        return;
    }
    const std::string patientId = toStd(shown.section(" - ", 0, 0));

    try {
        const std::string id = controller_.scheduleAppointment(
            patientId,
            toStd(dateEdit_->date().toString("yyyy-MM-dd")),
            toStd(timeEdit_->time().toString("HH:mm")),
            toStd(serviceEdit_->text()));
        QMessageBox::information(this, "Appointment Scheduled",
                                 toQt("Appointment " + id + " created."));
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Scheduling Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void AppointmentPanel::onCancelClicked()
{
    const std::string id = selectedAppointmentId();
    if (id.empty()) {
        QMessageBox::information(this, "No Selection",
                                 "Select an appointment row first.");
        return;
    }
    const auto answer = QMessageBox::question(
        this, "Confirm Cancel",
        toQt("Cancel appointment " + id + "? (Undo can restore it.)"));
    if (answer != QMessageBox::Yes)
        return;

    try {
        controller_.cancelAppointment(id);
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Cancel Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void AppointmentPanel::onCompleteClicked()
{
    const std::string id = selectedAppointmentId();
    if (id.empty()) {
        QMessageBox::information(this, "No Selection",
                                 "Select an appointment row first.");
        return;
    }
    try {
        controller_.completeAppointment(id);
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Update Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

} // namespace gui
} // namespace scapms
