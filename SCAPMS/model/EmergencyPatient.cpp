#include "EmergencyPatient.h"

#include <sstream>

namespace scapms {
namespace model {

EmergencyPatient::EmergencyPatient(const std::string& patientId,
                                   const std::string& name,
                                   int age,
                                   const std::string& contact,
                                   int severityLevel,
                                   const std::string& description)
    : PatientRecord(patientId, name, age, contact),
      severityLevel_(severityLevel),
      description_(description)
{
}

std::string EmergencyPatient::getTypeName() const
{
    return "Emergency";
}

int EmergencyPatient::getPriority() const
{
    // Emergency patients always have the highest consultation priority;
    // very severe cases get an extra boost when the queue is reordered.
    return 3 + (severityLevel_ >= 4 ? 1 : 0);
}

std::string EmergencyPatient::severityText() const
{
    switch (severityLevel_) {
    case 1: return "Minor";
    case 2: return "Low";
    case 3: return "Moderate";
    case 4: return "High";
    default: return "Critical";
    }
}

void EmergencyPatient::displayInfo(std::ostream& os) const
{
    os << "[EMERGENCY] " << patientId_ << " | " << name_
       << " | Age: " << age_ << " | Contact: " << contact_
       << " | Severity: " << severityText()
       << " | Case: " << (description_.empty() ? "-" : description_);
}

std::string EmergencyPatient::serialize() const
{
    std::ostringstream os;
    os << "EMERGENCY," << csvSafe(patientId_) << "," << csvSafe(name_) << ","
       << age_ << "," << csvSafe(contact_) << "," << severityLevel_ << ","
       << csvSafe(description_);
    return os.str();
}

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const
{
    return std::make_unique<EmergencyPatient>(*this);
}

bool EmergencyPatient::scheduleAppointment(const std::string& appointmentId,
                                           const std::string&,
                                           const std::string&)
{
    // Emergencies are accepted for an immediate/same-day slot without
    // checking availability - the controller flags the appointment urgent.
    addAppointmentId(appointmentId);
    return true;
}

bool EmergencyPatient::cancelAppointment(const std::string& appointmentId)
{
    // A severely ill emergency patient must NOT cancel the appointment;
    // the GUI is informed through the returned false value.
    if (severityLevel_ > 2)
        return false;
    if (!hasAppointment(appointmentId))
        return false;
    removeAppointmentId(appointmentId);
    return true;
}

} // namespace model
} // namespace scapms
