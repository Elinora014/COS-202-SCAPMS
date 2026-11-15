QT       += core gui widgets

CONFIG   += c++17
TARGET    = SCAPMS
TEMPLATE  = app

DEFINES  += QT_DEPRECATED_WARNINGS

# On systems where only the runtime libGL.so.1 exists (no libgl1-mesa-dev
# package), qmake's automatic "-lGL" cannot be resolved at link time.
# We create a libGL.so -> libGL.so.1 symlink inside the build directory
# before linking and add that directory to the library search path.
# (If you install libgl1-mesa-dev this block simply does nothing harmful;
# the line can also be removed.)
QMAKE_LIBDIR += $$OUT_PWD
QMAKE_PRE_LINK += ln -sf /usr/lib/x86_64-linux-gnu/libGL.so.1 $$OUT_PWD/libGL.so || true

# Model layer (pure C++ - no Qt dependency, keeps business logic portable)
HEADERS  += \
    model/Appointable.h \
    model/PatientRecord.h \
    model/GeneralPatient.h \
    model/EmergencyPatient.h \
    model/FollowUpPatient.h \
    model/Appointment.h \
    model/ClinicDatabase.h

SOURCES  += \
    model/PatientRecord.cpp \
    model/GeneralPatient.cpp \
    model/EmergencyPatient.cpp \
    model/FollowUpPatient.cpp \
    model/Appointment.cpp \
    model/ClinicDatabase.cpp

# Controller layer
HEADERS  += \
    controller/SearchEngine.h \
    controller/ClinicManager.h \
    controller/AppointmentController.h

SOURCES  += \
    controller/SearchEngine.cpp \
    controller/ClinicManager.cpp \
    controller/AppointmentController.cpp

# Utilities
HEADERS  += \
    utils/IDGenerator.h \
    utils/FileHandler.h \
    utils/Exceptions.h

SOURCES  += \
    utils/IDGenerator.cpp \
    utils/FileHandler.cpp

# GUI layer (Qt Widgets)
HEADERS  += \
    gui/MainWindow.h \
    gui/PatientPanel.h \
    gui/AppointmentPanel.h \
    gui/QueuePanel.h \
    gui/SearchSortPanel.h \
    gui/AdminPanel.h \
    gui/GuiHelpers.h

SOURCES  += \
    gui/MainWindow.cpp \
    gui/PatientPanel.cpp \
    gui/AppointmentPanel.cpp \
    gui/QueuePanel.cpp \
    gui/SearchSortPanel.cpp \
    gui/AdminPanel.cpp

SOURCES  += main.cpp
