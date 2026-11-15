#include "MainWindow.h"
#include "PatientPanel.h"
#include "AppointmentPanel.h"
#include "QueuePanel.h"
#include "SearchSortPanel.h"
#include "AdminPanel.h"
#include "GuiHelpers.h"

#include "../utils/Exceptions.h"

#include <QApplication>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QDate>
#include <QTime>

namespace scapms {
namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), appointments_(manager_)
{
    setWindowTitle("SCAPMS - Smart Clinic Appointment & Patient Management System");

    buildCentral();
    buildMenus();

    statusStatsLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusStatsLabel_);
    statusBar()->showMessage("Ready.");

    // ---------- timer events (requirement 6d) ----------
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_->start(5000); // 5 s dashboard / waiting-time refresh

    resize(1240, 760);

    // Auto-load the default data set when it exists.
    if (QFile::exists(dataDirectory_ + "/patients.csv"))
        loadFrom(dataDirectory_, /*quiet=*/true);
    else
        refreshAll();
}

// ---------- UI construction ----------

void MainWindow::buildCentral()
{
    // The central widget demonstrates plain QVBoxLayout management:
    // the tab widget always fills the window and resizes with it.
    tabs_ = new QTabWidget(this);

    patientPanel_     = new PatientPanel(manager_, tabs_);
    appointmentPanel_ = new AppointmentPanel(manager_, appointments_, tabs_);
    queuePanel_       = new QueuePanel(manager_, tabs_);
    searchPanel_      = new SearchSortPanel(manager_, tabs_);
    adminPanel_       = new AdminPanel(manager_, tabs_);

    tabs_->addTab(patientPanel_, "Patient Records");
    tabs_->addTab(appointmentPanel_, "Appointments");
    tabs_->addTab(queuePanel_, "Waiting Queue");
    tabs_->addTab(searchPanel_, "Search & Sort");
    tabs_->addTab(adminPanel_, "Admin & Reports");

    // Panels notify the shell about mutations through signals only.
    connect(patientPanel_, &PatientPanel::dataChanged,
            this, &MainWindow::onDataChanged);
    connect(appointmentPanel_, &AppointmentPanel::dataChanged,
            this, &MainWindow::onDataChanged);
    connect(queuePanel_, &QueuePanel::dataChanged,
            this, &MainWindow::onDataChanged);
    connect(searchPanel_, &SearchSortPanel::dataChanged,
            this, &MainWindow::onDataChanged);
    connect(adminPanel_, &AdminPanel::dataChanged,
            this, &MainWindow::onDataChanged);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(tabs_);
    setCentralWidget(central);
}

void MainWindow::buildMenus()
{
    // ---------------- File menu ----------------
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* newAct = fileMenu->addAction("&New (Clear Session)",
                                          this, &MainWindow::onNew);
    newAct->setShortcut(QKeySequence::New);           // Ctrl+N
    newAct->setToolTip("Start with an empty clinic database.");

    QAction* saveAct = fileMenu->addAction("&Save", this, &MainWindow::onSave);
    saveAct->setShortcut(QKeySequence::Save);         // Ctrl+S
    saveAct->setToolTip("Persist patients and appointments to CSV files.");

    QAction* loadAct = fileMenu->addAction("&Load", this, &MainWindow::onLoad);
    loadAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L)); // Ctrl+L
    loadAct->setToolTip("Read the last saved CSV data set.");

    fileMenu->addSeparator();

    QAction* exportAct = fileMenu->addAction("E&xport Data...",
                                             this, &MainWindow::onExport);
    exportAct->setToolTip("Choose a folder and write patients.csv / "
                          "appointments.csv there (file dialog).");

    QAction* importAct = fileMenu->addAction("&Import Data...",
                                             this, &MainWindow::onImport);
    importAct->setToolTip("Choose a folder containing the CSV files and "
                          "load them (file dialog).");

    fileMenu->addSeparator();

    QAction* exitAct = fileMenu->addAction("E&xit", this, &QWidget::close);
    exitAct->setShortcut(QKeySequence::Quit);         // Ctrl+Q
    exitAct->setToolTip("Close SCAPMS.");

    // ---------------- Edit menu ----------------
    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* undoAct = editMenu->addAction("&Undo Last Operation",
                                           this, &MainWindow::onUndo);
    undoAct->setShortcut(QKeySequence::Undo);         // Ctrl+Z
    undoAct->setToolTip("Reverse the most recent delete / modify / cancel.");

    // ---------------- View menu ----------------
    QMenu* viewMenu = menuBar()->addMenu("&View");
    QAction* searchAct = viewMenu->addAction("&Search Records",
                                             this, &MainWindow::onShowSearch);
    searchAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F)); // Ctrl+F
    searchAct->setToolTip("Jump to the Search & Sort tab and focus the "
                          "keyword field.");

    // ---------------- Help menu ----------------
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About SCAPMS", this, &MainWindow::about);
}

// ---------- slots ----------

void MainWindow::onDataChanged()
{
    refreshAll();
    statusBar()->showMessage(toQt("Data updated at " +
        QTime::currentTime().toString("HH:mm:ss").toStdString()), 4000);
}

void MainWindow::refreshAll()
{
    patientPanel_->refresh();
    appointmentPanel_->refresh();
    queuePanel_->refresh();
    searchPanel_->refresh();
    adminPanel_->refresh();
    adminPanel_->updateReminders(toStd(QDate::currentDate().toString("yyyy-MM-dd")));

    const auto s = manager_.computeStatistics();
    statusStatsLabel_->setText(toQt(
        "Patients: " + std::to_string(s.totalPatients) +
        " | Queue: " + std::to_string(s.waitingQueueSize) +
        " | Appointments: " + std::to_string(s.totalAppointments)));
}

void MainWindow::onNew()
{
    const auto answer = QMessageBox::question(
        this, "New Session",
        "Clear all patients and appointments from the current session?\n"
        "Saved files are not touched.");
    if (answer != QMessageBox::Yes)
        return;
    manager_.clearAll();
    refreshAll();
    statusBar()->showMessage("Session cleared.", 4000);
}

bool MainWindow::saveTo(const QString& directory, bool quiet)
{
    try {
        QDir().mkpath(directory);
        manager_.saveFiles(toStd(directory));
        if (!quiet)
            QMessageBox::information(this, "Save Successful",
                "Data saved into:\n" + directory + "/patients.csv\n" +
                directory + "/appointments.csv");
        statusBar()->showMessage(toQt("Saved to " + directory.toStdString()), 5000);
        return true;
    } catch (const utils::FileOperationError& e) {
        QMessageBox::critical(this, "Save Failed", toQt(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Save Failed", toQt(e.what()));
    }
    return false;
}

bool MainWindow::loadFrom(const QString& directory, bool quiet)
{
    try {
        const auto result = manager_.loadFiles(toStd(directory));

        QString message = toQt("Loaded " + std::to_string(result.patientCount) +
                               " patient(s) and " +
                               std::to_string(result.appointmentCount) +
                               " appointment(s).");
        if (!result.warnings.empty())
            message += toQt("\n" + std::to_string(result.warnings.size()) +
                            " malformed line(s) were skipped.");
        if (!quiet)
            QMessageBox::information(this, "Load Complete", message);
        refreshAll();
        statusBar()->showMessage(toQt("Loaded from " + directory.toStdString()), 5000);
        return true;
    } catch (const utils::FileOperationError& e) {
        if (!quiet)
            QMessageBox::critical(this, "Load Failed", toQt(e.what()));
        else
            statusBar()->showMessage("No saved data found in ./data.", 5000);
    } catch (const std::exception& e) {
        if (!quiet)
            QMessageBox::critical(this, "Load Failed", toQt(e.what()));
    }
    return false;
}

void MainWindow::onSave()  { saveTo(dataDirectory_); }
void MainWindow::onLoad()  { loadFrom(dataDirectory_); }

void MainWindow::onExport()
{
    // File chooser (requirement 9c): pick the destination folder.
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Export Clinic Data - choose a destination folder",
        QDir::homePath());
    if (dir.isEmpty())
        return; // user cancelled
    saveTo(dir);
}

void MainWindow::onImport()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Import Clinic Data - choose the folder containing "
               "patients.csv",
        QDir::homePath());
    if (dir.isEmpty())
        return;
    loadFrom(dir);
}

void MainWindow::onUndo()
{
    adminPanel_->onUndoClicked(); // reuses the same exception handling
}

void MainWindow::onShowSearch()
{
    tabs_->setCurrentWidget(searchPanel_);
    searchPanel_->focusSearch();
}

void MainWindow::onTick()
{
    ++tickCount_;

    // Dashboard statistics + waiting times refresh every tick (5 s).
    adminPanel_->refresh();
    queuePanel_->refresh();

    // Appointment reminders every 6th tick (30 s).
    if (tickCount_ % 6 == 0) {
        const std::string today =
            toStd(QDate::currentDate().toString("yyyy-MM-dd"));
        adminPanel_->updateReminders(today);

        const auto todays = appointments_.appointmentsOn(today);
        if (!todays.empty()) {
            statusBar()->showMessage(toQt(
                "Reminder: " + std::to_string(todays.size()) +
                " appointment(s) scheduled for today (" + today + ")."), 6000);
        }

        // Alert staff about patients waiting longer than 10 minutes.
        if (manager_.frontWaitSeconds() > 600 && tickCount_ - lastWaitAlertTick_ > 60) {
            lastWaitAlertTick_ = tickCount_;
            const std::string front = manager_.queueSnapshot().front();
            statusBar()->showMessage(toQt(
                "Waiting alert: patient " + front +
                " has been waiting over 10 minutes!"), 8000);
        }
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, "About SCAPMS",
        "<h3>SCAPMS 1.0</h3>"
        "<p>Smart Clinic Appointment &amp; Patient Management System.</p>"
        "<p>Patient registration, appointments, waiting queue, undo stack, "
        "manual search/sort algorithms, CSV persistence and clinic reports.</p>"
        "<p>Shortcuts: Ctrl+S save | Ctrl+L load | Ctrl+F search | "
        "Ctrl+Z undo | Ctrl+Q exit</p>");
}

} // namespace gui
} // namespace scapms
