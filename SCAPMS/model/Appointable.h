#ifndef SCAPMS_MODEL_APPOINTABLE_H
#define SCAPMS_MODEL_APPOINTABLE_H

#include <string>

namespace scapms {
namespace model {

/**
 * Appointable - an interface-like abstract class.
 *
 * It defines the operations that any schedulable entity (here, the patient
 * types) must provide. Because it contains only pure virtual functions and
 * no data members, it behaves exactly like a Java/C# interface while using
 * standard C++.
 */
class Appointable {
public:
    virtual ~Appointable() = default;

    // Called when an appointment is scheduled for this entity.
    // Returns true when the scheduling is accepted by the patient type.
    virtual bool scheduleAppointment(const std::string& appointmentId,
                                     const std::string& date,
                                     const std::string& time) = 0;

    // Called when an appointment is cancelled for this entity.
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;
};

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_APPOINTABLE_H
