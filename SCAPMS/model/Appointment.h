#ifndef SCAPMS_MODEL_APPOINTMENT_H
#define SCAPMS_MODEL_APPOINTMENT_H

#include <string>

namespace scapms {
namespace model {

enum class AppointmentStatus { Scheduled, Completed, Cancelled };

/**
 * Appointment - information about one clinic appointment.
 *
 * Appointments are linked to patients through the patient ID and are
 * stored in the ClinicDatabase (association/composition demonstrated by
 * PatientRecord::appointmentIds_).
 *
 * Dates use the ISO "YYYY-MM-DD" format and times "HH:MM", so plain
 * lexicographic string comparison orders them chronologically - this is
 * what the manual sorting and binary searching code relies on.
 */
class Appointment {
public:
    Appointment() = default;
    Appointment(const std::string& id,
                const std::string& patientId,
                const std::string& date,
                const std::string& time,
                const std::string& service,
                AppointmentStatus status = AppointmentStatus::Scheduled);

    const std::string& getId()        const { return id_; }
    const std::string& getPatientId() const { return patientId_; }
    const std::string& getDate()      const { return date_; }
    const std::string& getTime()      const { return time_; }
    const std::string& getService()   const { return service_; }
    AppointmentStatus  getStatus()    const { return status_; }

    void setStatus(AppointmentStatus s) { status_ = s; }
    void setDate(const std::string& d)  { date_ = d; }
    void setTime(const std::string& t)  { time_ = t; }

    // "2026-01-15 09:30" - used for sorting/display.
    std::string dateTime() const { return date_ + " " + time_; }

    static std::string statusText(AppointmentStatus s);
    static AppointmentStatus statusFromText(const std::string& text);

    std::string serialize() const;   // CSV line
    static bool deserialize(const std::string& line, Appointment& out);

private:
    std::string        id_;
    std::string        patientId_;
    std::string        date_;
    std::string        time_;
    std::string        service_;
    AppointmentStatus  status_ = AppointmentStatus::Scheduled;
};

} // namespace model
} // namespace scapms

#endif // SCAPMS_MODEL_APPOINTMENT_H
