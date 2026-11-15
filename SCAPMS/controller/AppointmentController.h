#ifndef SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
#define SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H

#include <string>
#include <vector>

#include "../model/Appointment.h"
#include "ClinicManager.h"

namespace scapms {
namespace controller {

/**
 * AppointmentController - all appointment related business operations.
 *
 * It works together with ClinicManager (validation + undo registration)
 * and the ClinicDatabase (appointment storage), and demonstrates the
 * Appointable interface: scheduling/cancelling always goes through the
 * polymorphic patient->scheduleAppointment()/cancelAppointment() calls,
 * so every patient type can accept or refuse the operation with its own
 * behaviour.
 */
class AppointmentController {
public:
    explicit AppointmentController(ClinicManager& manager);

    // Validates the input, creates the Appointment, associates it with
    // the patient through the Appointable interface. Returns the new ID.
    // Throws utils::InvalidAppointmentData / utils::ClinicError.
    std::string scheduleAppointment(const std::string& patientId,
                                    const std::string& date,
                                    const std::string& time,
                                    const std::string& service);

    // Cancels through the patient (an EmergencyPatient with high severity
    // refuses cancellation). The record is kept with status Cancelled and
    // registered in the Undo stack.
    void cancelAppointment(const std::string& appointmentId);

    // Marks an appointment as consulted (kept for the reports).
    void completeAppointment(const std::string& appointmentId);

    // All appointments of one patient (any status).
    std::vector<const model::Appointment*> appointmentsOf(
        const std::string& patientId) const;

    // Appointment IDs scheduled for a specific calendar date - used by
    // the GUI reminder timer.
    std::vector<const model::Appointment*> appointmentsOn(
        const std::string& date) const;

    // Input validators (exposed so the GUI can pre-check a form).
    static bool isValidDate(const std::string& date);   // YYYY-MM-DD
    static bool isValidTime(const std::string& time);   // HH:MM

private:
    ClinicManager& manager_;
};

} // namespace controller
} // namespace scapms

#endif // SCAPMS_CONTROLLER_APPOINTMENTCONTROLLER_H
