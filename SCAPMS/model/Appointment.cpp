#include "Appointment.h"

#include <sstream>
#include "PatientRecord.h" // csvSafe / splitCsv helpers

namespace scapms {
namespace model {

Appointment::Appointment(const std::string& id,
                         const std::string& patientId,
                         const std::string& date,
                         const std::string& time,
                         const std::string& service,
                         AppointmentStatus status)
    : id_(id), patientId_(patientId), date_(date),
      time_(time), service_(service), status_(status)
{
}

std::string Appointment::statusText(AppointmentStatus s)
{
    switch (s) {
    case AppointmentStatus::Scheduled: return "Scheduled";
    case AppointmentStatus::Completed: return "Completed";
    case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Scheduled";
}

AppointmentStatus Appointment::statusFromText(const std::string& text)
{
    if (text == "Completed") return AppointmentStatus::Completed;
    if (text == "Cancelled") return AppointmentStatus::Cancelled;
    return AppointmentStatus::Scheduled;
}

std::string Appointment::serialize() const
{
    std::ostringstream os;
    os << csvSafe(id_) << "," << csvSafe(patientId_) << ","
       << csvSafe(date_) << "," << csvSafe(time_) << ","
       << csvSafe(service_) << "," << statusText(status_);
    return os.str();
}

bool Appointment::deserialize(const std::string& line, Appointment& out)
{
    std::vector<std::string> f = splitCsv(line);
    if (f.size() < 6)
        return false;
    out = Appointment(f[0], f[1], f[2], f[3], f[4], statusFromText(f[5]));
    return true;
}

} // namespace model
} // namespace scapms
