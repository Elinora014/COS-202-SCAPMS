#include "FollowUpPatient.h"

#include <sstream>

namespace scapms {
namespace model {

FollowUpPatient::FollowUpPatient(const std::string& patientId,
                                 const std::string& name,
                                 int age,
                                 const std::string& contact,
                                 const std::string& lastVisitDate,
                                 const std::string& reviewNote)
    : PatientRecord(patientId, name, age, contact),
      lastVisitDate_(lastVisitDate),
      reviewNote_(reviewNote)
{
}

std::string FollowUpPatient::getTypeName() const
{
    return "Follow-Up";
}

int FollowUpPatient::getPriority() const
{
    return 2; // between General (1) and Emergency (3+)
}

void FollowUpPatient::displayInfo(std::ostream& os) const
{
    os << "[FOLLOW-UP] " << patientId_ << " | " << name_
       << " | Age: " << age_ << " | Contact: " << contact_
       << " | Last visit: " << (lastVisitDate_.empty() ? "-" : lastVisitDate_)
       << " | Visit #" << (followUpCount_ + 1)
       << " | Note: " << (reviewNote_.empty() ? "-" : reviewNote_);
}

std::string FollowUpPatient::serialize() const
{
    std::ostringstream os;
    os << "FOLLOWUP," << csvSafe(patientId_) << "," << csvSafe(name_) << ","
       << age_ << "," << csvSafe(contact_) << ","
       << csvSafe(lastVisitDate_) << "," << csvSafe(reviewNote_) << ","
       << followUpCount_;
    return os.str();
}

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const
{
    return std::make_unique<FollowUpPatient>(*this);
}

bool FollowUpPatient::scheduleAppointment(const std::string& appointmentId,
                                          const std::string& date,
                                          const std::string&)
{
    if (hasAppointment(appointmentId))
        return false;
    // Scheduling a new appointment means the previous visit is now history
    // and one more follow-up visit is planned.
    if (!date.empty())
        lastVisitDate_ = date;
    ++followUpCount_;
    addAppointmentId(appointmentId);
    return true;
}

bool FollowUpPatient::cancelAppointment(const std::string& appointmentId)
{
    if (!hasAppointment(appointmentId))
        return false;
    removeAppointmentId(appointmentId);
    if (followUpCount_ > 0)
        --followUpCount_;
    return true;
}

} // namespace model
} // namespace scapms
