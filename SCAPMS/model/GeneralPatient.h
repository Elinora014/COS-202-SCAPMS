#ifndef SCAPMS_MODEL_GENERALPATIENT_H
#define SCAPMS_MODEL_GENERALPATIENT_H

#include "PatientRecord.h"

namespace scapms {
namespace model {

/**
 * GeneralPatient - routine patient type.
 *
 * Behaviour: standard priority (1), books ordinary appointments and may
 * freely cancel them. Holds the reason of the visit.
 */
class GeneralPatient : public PatientRecord {
public:
    GeneralPatient(const std::string& patientId,
                   const std::string& name,
                   int age,
                   const std::string& contact,
                   const std::string& reasonForVisit = "");

    std::string getTypeName() const override;
    int         getPriority() const override;
    void        displayInfo(std::ostream& os) const override;
    std::string serialize() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable implementation - general patients use the standard flow.
    bool scheduleAppointment(const std::string& appointmentId,
                             const std::string& date,
                             const std::string& time) override;
    bool cancelAppointment(const std::string& appointmentId) override;

    const std::string& reasonForVisit() const { return reasonForVisit_; }
    void setReasonForVisit(const std::string& reason) { reasonForVisit_ = reason; }

private:
    std::string reasonForVisit_;
};

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_GENERALPATIENT_H
