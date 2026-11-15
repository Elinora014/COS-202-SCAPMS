#ifndef SCAPMS_GUI_MAINWINDOW_H
#define SCAPMS_GUI_MAINWINDOW_H

#include <QMainWindow>

#include "../controller/ClinicManager.h"
#include "../controller/AppointmentController.h"

class QTabWidget;
class QLabel;
class QTimer;

namespace scapms {
namespace gui {

class PatientPanel;
class AppointmentPanel;
class QueuePanel;
class SearchSortPanel;
class AdminPanel;

/**
 * MainWindow - shell of the application.
 *
 * Owns the controller objects and the tab panels; the panels never talk
 * to each other directly - they emit dataChanged() and the window
 * refreshes everybody (event-driven communication, requirement 6).
 *
 * Layout management: the central widget uses a QVBoxLayout around the
 * QTabWidget, and every panel composes itself from QHBoxLayout,
 * QGridLayout, QFormLayout and QStackedWidget - no fixed geometry.
 *
 * Timers (requirements 6d / 9d): one 5-second QTimer refreshes the
 * dashboard statistics and waiting-time alerts, and every sixth tick
 * (30 s) re-checks today's appointment reminders.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onDataChanged();
    void onNew();
    void onSave();
    void onLoad();
    void onExport();
    void onImport();
    void onUndo();
    void onShowSearch();   // Ctrl+F
    void onTick();
    void about();

private:
    void buildMenus();
    void buildCentral();
    void refreshAll();
    bool saveTo(const QString& directory, bool quiet = false);
    bool loadFrom(const QString& directory, bool quiet = false);

    controller::ClinicManager manager_;
    controller::AppointmentController appointments_;

    QTabWidget* tabs_ = nullptr;
    PatientPanel* patientPanel_ = nullptr;
    AppointmentPanel* appointmentPanel_ = nullptr;
    QueuePanel* queuePanel_ = nullptr;
    SearchSortPanel* searchPanel_ = nullptr;
    AdminPanel* adminPanel_ = nullptr;

    QLabel* statusStatsLabel_ = nullptr;   // permanent status bar widget
    QTimer* timer_ = nullptr;
    int tickCount_ = 0;
    int lastWaitAlertTick_ = -100;

    QString dataDirectory_ = "data";       // default Save/Load location
};

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_MAINWINDOW_H
