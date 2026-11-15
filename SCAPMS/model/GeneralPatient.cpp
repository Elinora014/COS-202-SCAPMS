#include "GeneralPatient.h"

#include <sstream>

namespace scapms {
namespace model {

GeneralPatient::GeneralPatient(const std::string& patientId,
                               const std::string& name,
                               int age,
                               const std::string& contact,
                               const std::string& reasonForVisit)
    : PatientRecord(patientId, name, age, contact),
      reasonForVisit_(reasonForVisit)
{
}

std::string GeneralPatient::getTypeName() const
{
    return "General";
}

int GeneralPatient::getPriority() const
{
    return 1; // normal priority - served in FIFO order
}

void GeneralPatient::displayInfo(std::ostream& os) const
{
    os << "[GENERAL]   " << patientId_ << " | " << name_
       << " | Age: " << age_ << " | Contact: " << contact_
       << " | Reason: " << (reasonForVisit_.empty() ? "-" : reasonForVisit_);
}

std::string GeneralPatient::serialize() const
{
    std::ostringstream os;
    os << "GENERAL," << csvSafe(patientId_) << "," << csvSafe(name_) << ","
       << age_ << "," << csvSafe(contact_) << "," << csvSafe(reasonForVisit_);
    return os.str();
}

std::unique_ptr<PatientRecord> GeneralPatient::clone() const
{
    return std::make_unique<GeneralPatient>(*this);
}

bool GeneralPatient::scheduleAppointment(const std::string& appointmentId,
                                         const std::string&,
                                         const std::string&)
{
    // General patients can always be scheduled into a standard slot.
    addAppointmentId(appointmentId);
    return true;
}

bool GeneralPatient::cancelAppointment(const std::string& appointmentId)
{
    // General patients may freely cancel any of their appointments.
    if (!hasAppointment(appointmentId))
        return false;
    removeAppointmentId(appointmentId);
    return true;
}

} // namespace model
} // namespace scapms
