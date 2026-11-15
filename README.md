# SCAPMS - Smart Clinic Appointment & Patient Management System
Project Design Document

============================================================
## i. PROJECT DESCRIPTION
============================================================

SCAPMS (Smart Clinic Appointment & Patient Management System) is a desktop
application built with Qt 5 (C++) that manages patient registration,
appointment scheduling, a FIFO waiting queue, manual searching/sorting of
patient records, undoable administrative operations, and CSV-based
persistence.

The system is architected in three clean layers so business logic stays
independent of the GUI framework:

Layer        Directory      Responsibility
-------      ---------      --------------
Model        model/         Pure C++ data classes (patients, appointments,
                            database). No Qt dependency.
Controller   controller/    Business logic: ClinicManager,
                            AppointmentController, SearchEngine.
GUI          gui/           Qt Widgets tabs/panels that call controllers
                            only, never each other directly.

The entry point is main.cpp, which constructs a QApplication, shows
MainWindow, and runs the Qt event loop.
