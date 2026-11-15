#include "PatientRecord.h"

#include <algorithm>
#include <sstream>

namespace scapms {
namespace model {

PatientRecord::PatientRecord(const std::string& patientId,
                             const std::string& name,
                             int age,
                             const std::string& contact)
    : patientId_(patientId), name_(name), age_(age), contact_(contact)
{
}

void PatientRecord::addAppointmentId(const std::string& appointmentId)
{
    if (!hasAppointment(appointmentId))
        appointmentIds_.push_back(appointmentId);
}

void PatientRecord::removeAppointmentId(const std::string& appointmentId)
{
    appointmentIds_.erase(
        std::remove(appointmentIds_.begin(), appointmentIds_.end(), appointmentId),
        appointmentIds_.end());
}

bool PatientRecord::hasAppointment(const std::string& appointmentId) const
{
    return std::find(appointmentIds_.begin(), appointmentIds_.end(), appointmentId)
           != appointmentIds_.end();
}

std::string csvSafe(const std::string& field)
{
    std::string out;
    out.reserve(field.size());
    for (char c : field) {
        if (c == ',' || c == '\n' || c == '\r')
            out += ';';
        else
            out += c;
    }
    return out;
}

std::vector<std::string> splitCsv(const std::string& line)
{
    std::vector<std::string> fields;
    std::string current;
    std::istringstream ss(line);
    while (std::getline(ss, current, ','))
        fields.push_back(current);
    // A trailing comma means the last field is empty but still counts.
    if (!line.empty() && line.back() == ',')
        fields.push_back("");
    return fields;
}

} // namespace model
} // namespace scapms
