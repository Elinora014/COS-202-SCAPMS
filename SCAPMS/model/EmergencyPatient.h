#ifndef SCAPMS_MODEL_EMERGENCYPATIENT_H
#define SCAPMS_MODEL_EMERGENCYPATIENT_H

#include "PatientRecord.h"

namespace scapms {
namespace model {

/**
 * EmergencyPatient - high priority patient type.
 *
 * Behaviour: priority 3 (jumps the normal queue when called), carries a
 * severity level (1 = minor ... 5 = critical). Emergency patients are
 * scheduled immediately (same-day slot) and cancelling an appointment is
 * only allowed while the severity is low (<= 2).
 */
class EmergencyPatient : public PatientRecord {
public:
    EmergencyPatient(const std::string& patientId,
                     const std::string& name,
                     int age,
                     const std::string& contact,
                     int severityLevel = 3,
                     const std::string& description = "");

    std::string getTypeName() const override;
    int         getPriority() const override;
    void        displayInfo(std::ostream& os) const override;
    std::string serialize() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable implementation - emergencies get an immediate slot,
    // and cannot cancel while their condition is severe.
    bool scheduleAppointment(const std::string& appointmentId,
                             const std::string& date,
                             const std::string& time) override;
    bool cancelAppointment(const std::string& appointmentId) override;

    int severityLevel() const { return severityLevel_; }
    void setSeverityLevel(int level) { severityLevel_ = level; }

    const std::string& description() const { return description_; }
    void setDescription(const std::string& d) { description_ = d; }

    // Severity rendered as text for the GUI, e.g. "Critical".
    std::string severityText() const;

private:
    int         severityLevel_; // 1..5
    std::string description_;
};

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_EMERGENCYPATIENT_H
