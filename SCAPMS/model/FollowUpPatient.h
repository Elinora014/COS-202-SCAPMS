#ifndef SCAPMS_MODEL_FOLLOWUPPATIENT_H
#define SCAPMS_MODEL_FOLLOWUPPATIENT_H

#include "PatientRecord.h"

namespace scapms {
namespace model {

/**
 * FollowUpPatient - returning patient type.
 *
 * Behaviour: medium priority (2), remembers the previous visit date and
 * the doctor's review note. Appointments are handled normally but the
 * type keeps track of how many follow-up visits were recorded so the
 * GUI can show "Visit N".
 */
class FollowUpPatient : public PatientRecord {
public:
    FollowUpPatient(const std::string& patientId,
                    const std::string& name,
                    int age,
                    const std::string& contact,
                    const std::string& lastVisitDate = "",
                    const std::string& reviewNote = "");

    std::string getTypeName() const override;
    int         getPriority() const override;
    void        displayInfo(std::ostream& os) const override;
    std::string serialize() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable implementation - a scheduled appointment is treated as
    // the next recorded follow-up visit.
    bool scheduleAppointment(const std::string& appointmentId,
                             const std::string& date,
                             const std::string& time) override;
    bool cancelAppointment(const std::string& appointmentId) override;

    const std::string& lastVisitDate() const { return lastVisitDate_; }
    void setLastVisitDate(const std::string& d) { lastVisitDate_ = d; }

    const std::string& reviewNote() const { return reviewNote_; }
    void setReviewNote(const std::string& n) { reviewNote_ = n; }

    int followUpCount() const { return followUpCount_; }
    void setFollowUpCount(int count) { followUpCount_ = count; }

private:
    std::string lastVisitDate_;
    std::string reviewNote_;
    int         followUpCount_ = 0;
};

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_FOLLOWUPPATIENT_H
