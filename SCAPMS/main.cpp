// SCAPMS - Smart Clinic Appointment & Patient Management System
// Entry point: creates the Qt application and shows the main window.

#include <QApplication>

#include "gui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SCAPMS");
    QApplication::setOrganizationName("University Clinic");

    scapms::gui::MainWindow window;
    window.show();

    return app.exec();
}
