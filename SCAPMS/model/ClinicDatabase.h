#ifndef SCAPMS_MODEL_CLINICDATABASE_H
#define SCAPMS_MODEL_CLINICDATABASE_H

#include <vector>
#include <memory>

#include "PatientRecord.h"
#include "Appointment.h"

namespace scapms {
namespace model {

/**
 * ClinicDatabase - encapsulates the whole clinic data set.
 *
 * Uses a std::vector to store the registered patients (ownership through
 * std::unique_ptr so records are always released correctly) and another
 * std::vector for appointments. The GUI/controllers only see non-owning
 * raw pointers / references, so nobody can accidentally double-free.
 */
class ClinicDatabase {
public:
    // ---- patients ----
    void addPatient(PatientPtr patient);
    // Takes ownership of the removed patient back out of the database.
    PatientPtr removePatient(const std::string& patientId);
    // Re-inserts an existing record (used by the Undo mechanism).
    void insertPatient(PatientPtr patient);

    PatientRecord* findPatientRaw(const std::string& patientId);
    const PatientRecord* findPatientRaw(const std::string& patientId) const;

    // Direct access to the backing vector (indices are the table rows).
    std::vector<PatientPtr>& patients() { return patients_; }
    const std::vector<PatientPtr>& patients() const { return patients_; }
    std::size_t patientCount() const { return patients_.size(); }

    // ---- appointments ----
    void addAppointment(const Appointment& appt);
    bool removeAppointment(const std::string& appointmentId);
    Appointment* findAppointment(const std::string& appointmentId);
    const Appointment* findAppointment(const std::string& appointmentId) const;
    std::vector<Appointment>& appointments() { return appointments_; }
    const std::vector<Appointment>& appointments() const { return appointments_; }

    void clear();

private:
    std::vector<PatientPtr>  patients_;    // std::vector <unique_ptr<...>>
    std::vector<Appointment> appointments_;
};

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_CLINICDATABASE_H
