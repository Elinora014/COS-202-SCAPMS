#include "QueuePanel.h"
#include "GuiHelpers.h"

#include "../utils/Exceptions.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QGroupBox>

namespace scapms {
namespace gui {

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager)
{
    buildUi();
    refreshPatients();
    refresh();
}

void QueuePanel::buildUi()
{
    patientCombo_ = new QComboBox(this);
    patientCombo_->setToolTip("Pick a registered patient to add to the "
                              "waiting queue.");

    enqueueButton_ = new QPushButton("Add to Queue", this);
    enqueueButton_->setToolTip("Join the back of the FIFO queue.");

    callNextButton_ = new QPushButton("Call Next Patient", this);
    callNextButton_->setToolTip("Take the patient at the front of the "
                                "queue (FIFO) and start consultation.");

    connect(enqueueButton_, &QPushButton::clicked,
            this, &QueuePanel::onEnqueueClicked);
    connect(callNextButton_, &QPushButton::clicked,
            this, &QueuePanel::onCallNextClicked);

    frontLabel_ = new QLabel("Queue is empty.", this);
    frontLabel_->setStyleSheet("font-weight: bold;");

    table_ = new QTableWidget(this);
    table_->setColumnCount(5);
    QStringList headers;
    headers << "Position" << "Patient ID" << "Name" << "Category"
            << "Waiting Since";
    table_->setHorizontalHeaderLabels(headers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);

    auto* controls = new QHBoxLayout;
    controls->addWidget(patientCombo_, 2);
    controls->addWidget(enqueueButton_);
    controls->addWidget(callNextButton_);

    auto* group = new QGroupBox("Waiting Queue (FIFO - std::queue)", this);
    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->addLayout(controls);   // nested layout: row inside column
    groupLayout->addWidget(frontLabel_);
    groupLayout->addWidget(table_, 1);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->addWidget(group, 1);
}

void QueuePanel::refreshPatients()
{
    const QString previous = patientCombo_->currentText();
    patientCombo_->blockSignals(true);
    patientCombo_->clear();
    for (const auto& p : manager_.database().patients()) {
        if (!manager_.isInQueue(p->getPatientId()))
            patientCombo_->addItem(toQt(p->getPatientId() + " - " + p->getName()));
    }
    const int idx = previous.isEmpty() ? -1 : patientCombo_->findText(previous);
    if (idx >= 0)
        patientCombo_->setCurrentIndex(idx);
    patientCombo_->blockSignals(false);
}

void QueuePanel::refresh()
{
    refreshPatients();

    const std::vector<std::string> ids = manager_.queueSnapshot();
    table_->setRowCount(0);

    for (std::size_t i = 0; i < ids.size(); ++i) {
        const model::PatientRecord* p =
            manager_.database().findPatientRaw(ids[i]);
        const int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, plainItem(QString::number(i + 1)));
        table_->setItem(row, 1, plainItem(toQt(ids[i])));
        table_->setItem(row, 2, plainItem(p ? toQt(p->getName()) : "?"));
        table_->setItem(row, 3, plainItem(p ? toQt(p->getTypeName()) : "?"));

        const long long waited = manager_.waitSecondsAt(i);
        const int minutes = static_cast<int>(waited / 60);
        const int seconds = static_cast<int>(waited % 60);
        QString waitText = QString("%1:%2")
            .arg(minutes).arg(seconds, 2, 10, QChar('0')) + " min";
        table_->setItem(row, 4, plainItem(waitText));

        if (p)
            paintRow(table_, row, colorForPatientType(p->getTypeName()));
        // Alert staff about long waiting patients (requirement 6d/9d):
        if (waited > 10 * 60) {
            for (int col = 0; col < table_->columnCount(); ++col)
                if (auto* item = table_->item(row, col))
                    item->setForeground(Qt::darkRed);
        }
    }

    if (ids.empty()) {
        frontLabel_->setText("Queue is empty.");
    } else {
        const long long frontWait = manager_.frontWaitSeconds();
        frontLabel_->setText(toQt("Front of queue: " + ids.front() +
                                  "  (waiting " + std::to_string(frontWait / 60) +
                                  " min)"));
    }
    table_->resizeColumnsToContents();
}

void QueuePanel::onEnqueueClicked()
{
    const QString shown = patientCombo_->currentText();
    if (shown.isEmpty()) {
        QMessageBox::warning(this, "No Patient",
                             "There is no patient available to queue.");
        return;
    }
    try {
        manager_.enqueuePatient(toStd(shown.section(" - ", 0, 0)));
        emit dataChanged();
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Queue Error", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

void QueuePanel::onCallNextClicked()
{
    try {
        const std::string id = manager_.callNextPatient();
        const model::PatientRecord* p = manager_.database().findPatientRaw(id);
        QMessageBox::information(
            this, "Now Serving",
            toQt("Please call patient " + id +
                 (p ? " (" + p->getName() + ")" : "")));
        emit dataChanged();
    } catch (const utils::QueueEmptyError& e) {
        QMessageBox::warning(this, "Empty Queue", toQt(e.what()));
    } catch (const utils::ClinicError& e) {
        QMessageBox::critical(this, "Queue Error", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Unexpected Error", toQt(e.what()));
    }
}

} // namespace gui
} // namespace scapms
