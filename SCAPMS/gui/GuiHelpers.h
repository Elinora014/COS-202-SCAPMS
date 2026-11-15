#ifndef SCAPMS_GUI_GUIHELPERS_H
#define SCAPMS_GUI_GUIHELPERS_H

#include <QString>
#include <QColor>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <string>

#include "../model/PatientRecord.h"
#include "../model/Appointment.h"
#include "../model/GeneralPatient.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"

namespace scapms {
namespace gui {

// ---------- string conversion helpers ----------
inline QString toQt(const std::string& s) { return QString::fromStdString(s); }
inline std::string toStd(const QString& s) { return s.toStdString(); }

// ---------- custom table presentation (requirement 9a) ----------
// Row background colour encodes the patient category so the categories
// are visually distinguishable inside every table of the application.
inline QColor colorForPatientType(const std::string& typeName)
{
    if (typeName == "Emergency")   return QColor(255, 224, 224); // red-ish
    if (typeName == "Follow-Up")   return QColor(224, 236, 255); // blue-ish
    return QColor(228, 255, 228);                               // green-ish
}

inline QColor colorForAppointmentStatus(model::AppointmentStatus s)
{
    switch (s) {
    case model::AppointmentStatus::Scheduled: return QColor(255, 249, 219);
    case model::AppointmentStatus::Completed: return QColor(220, 245, 220);
    case model::AppointmentStatus::Cancelled: return QColor(240, 240, 240);
    }
    return QColor(255, 255, 255);
}

// Creates a non-editable table item.
inline QTableWidgetItem* plainItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setForeground(Qt::black);
    return item;
}

inline void paintRow(QTableWidget* table, int row, const QColor& color)
{
    for (int col = 0; col < table->columnCount(); ++col)
        if (QTableWidgetItem* item = table->item(row, col))
            item->setBackground(color);
}

// Short type-specific detail text for a patient record (polymorphic
// down-casts kept in one place instead of scattered through the GUI).
inline std::string patientDetails(const model::PatientRecord* p)
{
    if (auto* e = dynamic_cast<const model::EmergencyPatient*>(p))
        return "Severity: " + e->severityText();
    if (auto* g = dynamic_cast<const model::GeneralPatient*>(p))
        return g->reasonForVisit().empty() ? "-" : g->reasonForVisit();
    if (auto* f = dynamic_cast<const model::FollowUpPatient*>(p))
        return "Last visit: " + (f->lastVisitDate().empty()
                                     ? std::string("-") : f->lastVisitDate());
    return "";
}

} // namespace gui
} // namespace scapms

#endif // SCAPMS_GUI_GUIHELPERS_H
