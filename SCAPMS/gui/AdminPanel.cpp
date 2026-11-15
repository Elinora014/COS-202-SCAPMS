#include "AdminPanel.h"
#include "GuiHelpers.h"

#include "../utils/Exceptions.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QMessageBox>

#include <sstream>

namespace scapms {
namespace gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    buildUi();
    refresh();
}

void AdminPanel::buildUi()
{
    // ---------- dashboard + undo ----------
    statsLabel_ = new QLabel(this);
    statsLabel_->setWordWrap(true);
    statsLabel_->setStyleSheet("QLabel { background:#f0f4f8; padding:10px; "
                               "border-radius:6px; }");

    undoButton_ = new QPushButton("Undo Last Administrative Operation", this);
    undoButton_->setToolTip("Reverses the most recent delete / modification "
                            "or appointment cancellation (Ctrl+Z).");
    undoHintLabel_ = new QLabel(this);
    undoHintLabel_->setWordWrap(true);
    connect(undoButton_, &QPushButton::clicked,
            this, &AdminPanel::onUndoClicked);

    auto* dashGroup = new QGroupBox("Dashboard & Undo", this);
    auto* dashLayout = new QVBoxLayout(dashGroup);
    dashLayout->addWidget(statsLabel_);
    dashLayout->addWidget(undoButton_);
    dashLayout->addWidget(undoHintLabel_);

    // ---------- frequently accessed records (array cache) ----------
    cacheTable_ = new QTableWidget(this);
    cacheTable_->setColumnCount(4);
    QStringList headers;
    headers << "#" << "Patient ID" << "Name" << "Category";
    cacheTable_->setHorizontalHeaderLabels(headers);
    cacheTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cacheTable_->verticalHeader()->setVisible(false);
    cacheTable_->horizontalHeader()->setStretchLastSection(true);

    auto* cacheGroup = new QGroupBox(
        "Frequently Accessed Patient Records (fixed array cache, max 10)",
        this);
    auto* cacheLayout = new QVBoxLayout(cacheGroup);
    cacheLayout->addWidget(cacheTable_);

    // ---------- report ----------
    reportButton_ = new QPushButton("Generate Clinic Report", this);
    reportButton_->setToolTip("Builds the report by calling displayInfo() "
                              "polymorphically on every record.");
    reportView_ = new QPlainTextEdit(this);
    reportView_->setReadOnly(true);
    reportView_->setStyleSheet("font-family: monospace;");
    connect(reportButton_, &QPushButton::clicked, this, [this]() {
        std::ostringstream os;
        os << "===== SCAPMS CLINIC REPORT =====\n\n";

        auto s = manager_.computeStatistics();
        os << "Registered patients : " << s.totalPatients << "\n"
           << "  General           : " << s.generalPatients << "\n"
           << "  Emergency         : " << s.emergencyPatients << "\n"
           << "  Follow-Up         : " << s.followUpPatients << "\n"
           << "Appointments        : " << s.totalAppointments
           << "  (scheduled " << s.scheduledAppointments
           << ", completed " << s.completedAppointments
           << ", cancelled " << s.cancelledAppointments << ")\n"
           << "Waiting queue       : " << s.waitingQueueSize << "\n\n"
           << "----- PATIENT LIST (polymorphic processing) -----\n";
        manager_.processAllPatients(os); // runtime polymorphism showcase
        reportView_->setPlainText(toQt(os.str()));
    });

    reminderLabel_ = new QLabel(this);
    reminderLabel_->setWordWrap(true);

    auto* reportGroup = new QGroupBox("Basic Clinic Reports", this);
    auto* reportLayout = new QVBoxLayout(reportGroup);
    reportLayout->addWidget(reportButton_);
    reportLayout->addWidget(reminderLabel_);
    reportLayout->addWidget(reportView_, 1);

    // ---------- layout: two columns (QHBoxLayout) ----------
    auto* left = new QVBoxLayout;
    left->addWidget(dashGroup);
    left->addWidget(cacheGroup);

    auto* leftWidget = new QWidget(this);
    leftWidget->setLayout(left);

    auto* split = new QHBoxLayout(this);
    split->setContentsMargins(8, 8, 8, 8);
    split->addWidget(leftWidget, 1);  // proportional stretch, no fixed geometry
    split->addWidget(reportGroup, 2);
}

void AdminPanel::refresh()
{
    const auto s = manager_.computeStatistics();
    std::ostringstream os;
    os << "Patients: " << s.totalPatients
       << "  (General " << s.generalPatients
       << " / Emergency " << s.emergencyPatients
       << " / Follow-Up " << s.followUpPatients << ")"
       << "   Appointments: " << s.totalAppointments
       << "  (Scheduled " << s.scheduledAppointments
       << " / Completed " << s.completedAppointments
       << " / Cancelled " << s.cancelledAppointments << ")"
       << "   Waiting queue: " << s.waitingQueueSize;
    statsLabel_->setText(toQt(os.str()));

    const std::string top = manager_.peekUndo();
    undoButton_->setEnabled(!top.empty());
    undoHintLabel_->setText(top.empty()
        ? "Nothing to undo."
        : toQt("Last operation: " + top));

    // Cache table.
    cacheTable_->setRowCount(0);
    int position = 1;
    for (const auto* p : manager_.recentRecords()) {
        const int row = cacheTable_->rowCount();
        cacheTable_->insertRow(row);
        cacheTable_->setItem(row, 0, plainItem(QString::number(position++)));
        cacheTable_->setItem(row, 1, plainItem(toQt(p->getPatientId())));
        cacheTable_->setItem(row, 2, plainItem(toQt(p->getName())));
        cacheTable_->setItem(row, 3, plainItem(toQt(p->getTypeName())));
        paintRow(cacheTable_, row, colorForPatientType(p->getTypeName()));
    }
    cacheTable_->resizeColumnsToContents();
}

void AdminPanel::updateReminders(const std::string& today)
{
    // Collect today's scheduled appointments straight from the database.
    std::vector<const model::Appointment*> todays;
    for (const auto& a : manager_.database().appointments()) {
        if (a.getStatus() == model::AppointmentStatus::Scheduled &&
            a.getDate() == today)
            todays.push_back(&a);
    }

    if (todays.empty()) {
        reminderLabel_->setText("Reminders: none for " + toQt(today) + ".");
        return;
    }

    QStringList parts;
    for (const auto* a : todays)
        parts << toQt(a->getTime() + " " + a->getPatientId() +
                      " - " + a->getService());
    reminderLabel_->setText("<b>Today's appointment reminders</b><br>" +
                            parts.join("<br>"));
}

void AdminPanel::onUndoClicked()
{
    try {
        const std::string undone = manager_.undoLast();
        QMessageBox::information(this, "Undo Successful",
                                 toQt("Reversed: " + undone +
                                      "\n(Note: undo steps through the "
                                      "operation stack one entry at a time.)"));
        emit dataChanged();
        refresh();
    } catch (const utils::ClinicError& e) {
        QMessageBox::warning(this, "Undo Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

} // namespace gui
} // namespace scapms
