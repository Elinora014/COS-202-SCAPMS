#ifndef SCAPMS_MODEL_PATIENTRECORD_H
#define SCAPMS_MODEL_PATIENTRECORD_H

#include <string>
#include <vector>
#include <memory>
#include <ostream>

#include "Appointable.h"

namespace scapms {
namespace model {

/**
 * PatientRecord - abstract base class of the patient hierarchy.
 *
 * Holds the common information of every patient (ID, name, age, contact)
 * with full encapsulation (private data + validated accessors) and declares
 * the pure virtual functions that each concrete patient type must implement.
 *
 * It also demonstrates COMPOSITION: a patient owns a list of the IDs of
 * its related Appointment objects (a patient is associated with one or
 * more appointments).
 *
 * The class is intentionally Qt-free so that the model/controller layers
 * stay independent of the GUI framework.
 */
class PatientRecord : public Appointable {
protected:
    std::string patientId_;
    std::string name_;
    int         age_;
    std::string contact_;

    // Composition: IDs of the appointments linked to this patient.
    std::vector<std::string> appointmentIds_;

    // Common constructor; validation is done by the derived classes / manager.
    PatientRecord(const std::string& patientId,
                  const std::string& name,
                  int age,
                  const std::string& contact);

public:
    virtual ~PatientRecord() = default;

    // ---- Common accessors (encapsulation) ----
    const std::string& getPatientId() const { return patientId_; }
    const std::string& getName()      const { return name_; }
    int                getAge()       const { return age_; }
    const std::string& getContact()   const { return contact_; }

    void setName(const std::string& name)         { name_ = name; }
    void setAge(int age)                          { age_ = age; }
    void setContact(const std::string& contact)   { contact_ = contact; }

    // ---- Appointment association (composition) ----
    const std::vector<std::string>& getAppointmentIds() const { return appointmentIds_; }
    void addAppointmentId(const std::string& appointmentId);
    void removeAppointmentId(const std::string& appointmentId);
    bool hasAppointment(const std::string& appointmentId) const;

    // ---- Pure virtual functions (make the class abstract) ----

    // Human readable category name, e.g. "Emergency".
    virtual std::string getTypeName() const = 0;

    // Consultation priority (higher value = served earlier).
    virtual int getPriority() const = 0;

    // Polymorphic self-description used by the report/summary code.
    virtual void displayInfo(std::ostream& os) const = 0;

    // Serialisation to a CSV line (used by FileHandler for persistence).
    virtual std::string serialize() const = 0;

    // Polymorphic deep copy - required for the Undo stack and the cache.
    virtual std::unique_ptr<PatientRecord> clone() const = 0;
};

// Convenience typedef used across the program.
using PatientPtr = std::unique_ptr<PatientRecord>;

// Turns a free-text field into a CSV-safe token (commas/new-lines removed)
// so that one patient record always occupies exactly one line in the file.
std::string csvSafe(const std::string& field);

// Splits one CSV line into fields (simple comma separation - values are
// guaranteed comma-free by csvSafe() before being written).
std::vector<std::string> splitCsv(const std::string& line);

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_PATIENTRECORD_H
