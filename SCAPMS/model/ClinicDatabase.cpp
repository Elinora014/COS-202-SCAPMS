#include "ClinicDatabase.h"

#include <algorithm>

namespace scapms {
namespace model {

void ClinicDatabase::addPatient(PatientPtr patient)
{
    patients_.push_back(std::move(patient));
}

PatientPtr ClinicDatabase::removePatient(const std::string& patientId)
{
    auto it = std::find_if(patients_.begin(), patients_.end(),
                           [&](const PatientPtr& p) {
                               return p->getPatientId() == patientId;
                           });
    if (it == patients_.end())
        return nullptr;

    PatientPtr taken = std::move(*it);
    patients_.erase(it);
    return taken;
}

void ClinicDatabase::insertPatient(PatientPtr patient)
{
    if (patient)
        patients_.push_back(std::move(patient));
}

PatientRecord* ClinicDatabase::findPatientRaw(const std::string& patientId)
{
    auto it = std::find_if(patients_.begin(), patients_.end(),
                           [&](const PatientPtr& p) {
                               return p->getPatientId() == patientId;
                           });
    return it == patients_.end() ? nullptr : it->get();
}

const PatientRecord* ClinicDatabase::findPatientRaw(const std::string& patientId) const
{
    auto it = std::find_if(patients_.begin(), patients_.end(),
                           [&](const PatientPtr& p) {
                               return p->getPatientId() == patientId;
                           });
    return it == patients_.end() ? nullptr : it->get();
}

void ClinicDatabase::addAppointment(const Appointment& appt)
{
    appointments_.push_back(appt);

    // Maintain the two-way association: the patient records the ID too.
    PatientRecord* p = findPatientRaw(appt.getPatientId());
    if (p)
        p->addAppointmentId(appt.getId());
}

bool ClinicDatabase::removeAppointment(const std::string& appointmentId)
{
    auto it = std::find_if(appointments_.begin(), appointments_.end(),
                           [&](const Appointment& a) {
                               return a.getId() == appointmentId;
                           });
    if (it == appointments_.end())
        return false;

    PatientRecord* p = findPatientRaw(it->getPatientId());
    if (p)
        p->removeAppointmentId(appointmentId);

    appointments_.erase(it);
    return true;
}

Appointment* ClinicDatabase::findAppointment(const std::string& appointmentId)
{
    auto it = std::find_if(appointments_.begin(), appointments_.end(),
                           [&](const Appointment& a) {
                               return a.getId() == appointmentId;
                           });
    return it == appointments_.end() ? nullptr : &(*it);
}

const Appointment* ClinicDatabase::findAppointment(const std::string& appointmentId) const
{
    auto it = std::find_if(appointments_.begin(), appointments_.end(),
                           [&](const Appointment& a) {
                               return a.getId() == appointmentId;
                           });
    return it == appointments_.end() ? nullptr : &(*it);
}

void ClinicDatabase::clear()
{
    patients_.clear();
    appointments_.clear();
}

} // namespace model
} // namespace scapms
