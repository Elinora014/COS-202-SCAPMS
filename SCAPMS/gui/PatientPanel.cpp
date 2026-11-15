#include "PatientPanel.h"
#include "GuiHelpers.h"

#include "../utils/Exceptions.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>

namespace scapms {
namespace gui {

// Combo indices <-> CSV type tokens.
static const char* kTypeTokens[] = { "GENERAL", "EMERGENCY", "FOLLOWUP" };

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    buildUi();
    refresh();
}

void PatientPanel::buildUi()
{
    // ---------- left: patient table ----------
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    QStringList headers;
    headers << "Patient ID" << "Name" << "Age" << "Contact"
            << "Category" << "Details";
    table_->setHorizontalHeaderLabels(headers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setStyleSheet("alternate-background-color: #f7f7f7;"
                          "selection-background-color: #4a90d9;");
    table_->setAlternatingRowColors(false);
    connect(table_, &QTableWidget::itemSelectionChanged,
            this, &PatientPanel::onSelectionChanged);

    // ---------- right: input form ----------
    idLabel_ = new QLineEdit(this);
    idLabel_->setReadOnly(true);
    idLabel_->setPlaceholderText("(auto-assigned)");

    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({"General", "Emergency", "Follow-Up"});
    typeCombo_->setToolTip("Patients of each category get different "
                           "priority and extra fields.");

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText("Full name");
    nameEdit_->setToolTip("Patient name must not be empty.");

    ageSpin_ = new QSpinBox(this);
    ageSpin_->setRange(0, 120); // input validation (requirement 9e)
    ageSpin_->setToolTip("Valid ages are between 0 and 120.");

    contactEdit_ = new QLineEdit(this);
    contactEdit_->setPlaceholderText("Phone / e-mail");

    // Dynamic type-specific pages (requirement 9b).
    extrasStack_ = new QStackedWidget(this);

    auto* generalPage = new QWidget(extrasStack_);
    auto* generalForm = new QFormLayout(generalPage);
    reasonEdit_ = new QLineEdit(generalPage);
    reasonEdit_->setPlaceholderText("Reason for visit");
    generalForm->addRow("Reason:", reasonEdit_);

    auto* emergencyPage = new QWidget(extrasStack_);
    auto* emergencyForm = new QFormLayout(emergencyPage);
    severityCombo_ = new QComboBox(emergencyPage);
    severityCombo_->addItems({"1 - Minor", "2 - Low", "3 - Moderate",
                              "4 - High", "5 - Critical"});
    severityCombo_->setCurrentIndex(2);
    descEdit_ = new QLineEdit(emergencyPage);
    descEdit_->setPlaceholderText("Case description");
    emergencyForm->addRow("Severity:", severityCombo_);
    emergencyForm->addRow("Case:", descEdit_);

    auto* followPage = new QWidget(extrasStack_);
    auto* followForm = new QFormLayout(followPage);
    lastVisitEdit_ = new QLineEdit(followPage);
    lastVisitEdit_->setPlaceholderText("YYYY-MM-DD");
    noteEdit_ = new QLineEdit(followPage);
    noteEdit_->setPlaceholderText("Doctor's review note");
    followForm->addRow("Last visit:", lastVisitEdit_);
    followForm->addRow("Note:", noteEdit_);

    extrasStack_->addWidget(generalPage);
    extrasStack_->addWidget(emergencyPage);
    extrasStack_->addWidget(followPage);

    connect(typeCombo_, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTypeChanged(int)));

    auto* form = new QFormLayout;
    form->addRow("ID:", idLabel_);
    form->addRow("Category:", typeCombo_);
    form->addRow("Name:", nameEdit_);
    form->addRow("Age:", ageSpin_);
    form->addRow("Contact:", contactEdit_);
    form->addRow("Details:", extrasStack_);

    addButton_ = new QPushButton("Add Patient", this);
    addButton_->setToolTip("Register a new patient (Ctrl+N is used by Qt - "
                           "use the button).");
    updateButton_ = new QPushButton("Update Selected", this);
    updateButton_->setToolTip("Save the form values into the selected record.");
    deleteButton_ = new QPushButton("Delete Selected", this);
    deleteButton_->setToolTip("Remove the selected patient "
                              "(reversible with Undo Last Operation).");
    updateButton_->setEnabled(false);
    deleteButton_->setEnabled(false);

    connect(addButton_, &QPushButton::clicked, this, &PatientPanel::onAddClicked);
    connect(updateButton_, &QPushButton::clicked, this, &PatientPanel::onUpdateClicked);
    connect(deleteButton_, &QPushButton::clicked, this, &PatientPanel::onDeleteClicked);

    hintLabel_ = new QLabel("Select a row to edit or delete.", this);
    hintLabel_->setWordWrap(true);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(addButton_);
    buttonRow->addWidget(updateButton_);
    buttonRow->addWidget(deleteButton_);

    auto* formGroup = new QGroupBox("Patient Information", this);
    auto* formOuter = new QVBoxLayout(formGroup);
    formOuter->addLayout(form);
    formOuter->addLayout(buttonRow);
    formOuter->addWidget(hintLabel_);

    auto* split = new QHBoxLayout(this);
    split->setContentsMargins(8, 8, 8, 8);
    // Table gets stretch 3, form gets stretch 2 -> proportional layout
    // instead of fixed geometry (requirement 8).
    split->addWidget(table_, 3);
    split->addWidget(formGroup, 2);
}

// ---------- form <-> selection ----------

void PatientPanel::onTypeChanged(int index)
{
    extrasStack_->setCurrentIndex(index); // dynamic component switch
}

void PatientPanel::onSelectionChanged()
{
    const int row = table_->currentRow();
    if (row < 0) {
        clearForm();
        return;
    }
    selectedPatientId_ = toStd(table_->item(row, 0)->text());
    model::PatientRecord* p = manager_.getPatient(selectedPatientId_);
    if (!p) {
        clearForm();
        return;
    }
    loadFormFromPatient(p);
    updateButton_->setEnabled(true);
    deleteButton_->setEnabled(true);
    hintLabel_->setText(toQt("Editing " + selectedPatientId_ + " (" +
                             p->getTypeName() + " patient)."));
}

void PatientPanel::loadFormFromPatient(const model::PatientRecord* patient)
{
    const std::string type = patient->getTypeName();
    idLabel_->setText(toQt(patient->getPatientId()));
    nameEdit_->setText(toQt(patient->getName()));
    ageSpin_->setValue(patient->getAge());
    contactEdit_->setText(toQt(patient->getContact()));

    int idx = 0;
    if (type == "Emergency") idx = 1;
    else if (type == "Follow-Up") idx = 2;
    typeCombo_->setCurrentIndex(idx); // triggers onTypeChanged

    if (const auto* g = dynamic_cast<const model::GeneralPatient*>(patient))
        reasonEdit_->setText(toQt(g->reasonForVisit()));
    if (const auto* e = dynamic_cast<const model::EmergencyPatient*>(patient)) {
        severityCombo_->setCurrentIndex(e->severityLevel() - 1);
        descEdit_->setText(toQt(e->description()));
    }
    if (const auto* f = dynamic_cast<const model::FollowUpPatient*>(patient)) {
        lastVisitEdit_->setText(toQt(f->lastVisitDate()));
        noteEdit_->setText(toQt(f->reviewNote()));
    }
}

void PatientPanel::clearForm()
{
    selectedPatientId_.clear();
    idLabel_->clear();
    nameEdit_->clear();
    ageSpin_->setValue(0);
    contactEdit_->clear();
    reasonEdit_->clear();
    descEdit_->clear();
    lastVisitEdit_->clear();
    noteEdit_->clear();
    updateButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
    hintLabel_->setText("Select a row to edit or delete.");
}

void PatientPanel::collectExtras(std::string& extra1, std::string& extra2) const
{
    switch (typeCombo_->currentIndex()) {
    case 0: // General
        extra1 = toStd(reasonEdit_->text());
        extra2 = "";
        break;
    case 1: // Emergency - combo index 0..4 = severity 1..5
        extra1 = std::to_string(severityCombo_->currentIndex() + 1);
        extra2 = toStd(descEdit_->text());
        break;
    case 2: // Follow-Up
        extra1 = toStd(lastVisitEdit_->text());
        extra2 = toStd(noteEdit_->text());
        break;
    }
}

// ---------- button events ----------

void PatientPanel::onAddClicked()
{
    try {
        std::string extra1, extra2;
        collectExtras(extra1, extra2);
        const std::string id = manager_.addPatient(
            kTypeTokens[typeCombo_->currentIndex()],
            toStd(nameEdit_->text()),
            ageSpin_->value(),
            toStd(contactEdit_->text()),
            extra1, extra2);

        QMessageBox::information(this, "Patient Registered",
                                 toQt("New patient registered with ID " + id + "."));
        clearForm();
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Input Error", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void PatientPanel::onUpdateClicked()
{
    if (selectedPatientId_.empty())
        return;
    try {
        std::string extra1, extra2;
        collectExtras(extra1, extra2);
        manager_.editPatient(selectedPatientId_,
                            toStd(nameEdit_->text()),
                            ageSpin_->value(),
                            toStd(contactEdit_->text()),
                            extra1, extra2);
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Update Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void PatientPanel::onDeleteClicked()
{
    if (selectedPatientId_.empty())
        return;

    const auto answer = QMessageBox::question(
        this, "Confirm Delete",
        toQt("Really delete patient " + selectedPatientId_ + "?\n"
             "You can reverse this with Undo Last Operation."));
    if (answer != QMessageBox::Yes)
        return;

    try {
        manager_.deletePatient(selectedPatientId_);
        clearForm();
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Delete Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

// ---------- table refresh ----------

void PatientPanel::refresh()
{
    table_->setRowCount(0);
    for (const auto& p : manager_.database().patients()) {
        const int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, plainItem(toQt(p->getPatientId())));
        table_->setItem(row, 1, plainItem(toQt(p->getName())));
        table_->setItem(row, 2, plainItem(QString::number(p->getAge())));
        table_->setItem(row, 3, plainItem(toQt(p->getContact())));
        table_->setItem(row, 4, plainItem(toQt(p->getTypeName()))); // virtual
        table_->setItem(row, 5, plainItem(toQt(patientDetails(p.get()))));

        // Custom presentation: colour rows by patient category.
        paintRow(table_, row, colorForPatientType(p->getTypeName()));
    }
    table_->resizeColumnsToContents();
    clearForm();
}

} // namespace gui
} // namespace scapms
