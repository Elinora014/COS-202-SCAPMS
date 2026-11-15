#include "AppointmentController.h"

#include <cctype>

#include "../model/PatientRecord.h"
#include "../utils/Exceptions.h"
#include "../utils/IDGenerator.h"

namespace scapms {
namespace controller {

using namespace scapms::model;
using scapms::utils::IDGenerator;

AppointmentController::AppointmentController(ClinicManager& manager)
    : manager_(manager)
{
}

// ---------- validation ----------

bool AppointmentController::isValidDate(const std::string& date)
{
    // Expected: YYYY-MM-DD
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
        return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9})
        if (!std::isdigit(static_cast<unsigned char>(date[i])))
            return false;

    const int y = std::stoi(date.substr(0, 4));
    const int m = std::stoi(date.substr(5, 2));
    const int d = std::stoi(date.substr(8, 2));
    if (m < 1 || m > 12 || d < 1)
        return false;

    static const int daysInMonth[] =
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int limit = daysInMonth[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        limit = 29;
    return d <= limit;
}

bool AppointmentController::isValidTime(const std::string& time)
{
    // Expected: HH:MM
    if (time.size() != 5 || time[2] != ':')
        return false;
    for (int i : {0, 1, 3, 4})
        if (!std::isdigit(static_cast<unsigned char>(time[i])))
            return false;
    const int h = std::stoi(time.substr(0, 2));
    const int mn = std::stoi(time.substr(3, 2));
    return h >= 0 && h <= 23 && mn >= 0 && mn <= 59;
}

// ---------- operations ----------

std::string AppointmentController::scheduleAppointment(
    const std::string& patientId,
    const std::string& date,
    const std::string& time,
    const std::string& service)
{
    if (!isValidDate(date))
        throw utils::InvalidAppointmentData(
            "Date must be a real calendar date in YYYY-MM-DD format (got: "
            + date + ").");
    if (!isValidTime(time))
        throw utils::InvalidAppointmentData(
            "Time must be in HH:MM 24-hour format (got: " + time + ").");
    if (service.empty())
        throw utils::InvalidAppointmentData("Service must not be empty.");

    PatientRecord* patient = manager_.database().findPatientRaw(patientId);
    if (!patient)
        throw utils::ClinicError("Patient not found: " + patientId);

    // Two patients cannot share the exact same slot.
    for (const Appointment& a : manager_.database().appointments()) {
        if (a.getStatus() == AppointmentStatus::Scheduled &&
            a.getDate() == date && a.getTime() == time)
            throw utils::InvalidAppointmentData(
                "The slot " + date + " " + time + " is already taken.");
    }

    const std::string id = IDGenerator::nextAppointmentId();
    Appointment appt(id, patientId, date, time, service);

    // RUNTIME POLYMORPHISM through the Appointable interface: the patient
    // type decides whether the appointment is accepted.
    if (!patient->scheduleAppointment(id, date, time))
        throw utils::InvalidAppointmentData(
            patient->getTypeName() + " patient refused the appointment.");

    manager_.database().addAppointment(appt);
    return id;
}

void AppointmentController::cancelAppointment(const std::string& appointmentId)
{
    ClinicDatabase& db = manager_.database();
    Appointment* appt = db.findAppointment(appointmentId);
    if (!appt)
        throw utils::ClinicError("Appointment not found: " + appointmentId);
    if (appt->getStatus() == AppointmentStatus::Cancelled)
        throw utils::ClinicError("Appointment " + appointmentId +
                                 " is already cancelled.");

    PatientRecord* patient = db.findPatientRaw(appt->getPatientId());

    // Snapshot BEFORE the change so Undo can restore the old state.
    const Appointment snapshot = *appt;

    // Appointable interface again - the patient type may refuse.
    if (patient && !patient->cancelAppointment(appointmentId))
        throw utils::ClinicError(
            patient->getTypeName() + " patient " + patient->getPatientId() +
            " cannot cancel this appointment (severity too high).");

    manager_.pushCancelledAppointment(appointmentId, snapshot,
                                      appt->getPatientId());
    appt->setStatus(AppointmentStatus::Cancelled);
}

void AppointmentController::completeAppointment(const std::string& appointmentId)
{
    Appointment* appt = manager_.database().findAppointment(appointmentId);
    if (!appt)
        throw utils::ClinicError("Appointment not found: " + appointmentId);
    if (appt->getStatus() != AppointmentStatus::Scheduled)
        throw utils::ClinicError("Only scheduled appointments can be completed.");
    appt->setStatus(AppointmentStatus::Completed);
}

std::vector<const Appointment*> AppointmentController::appointmentsOf(
    const std::string& patientId) const
{
    std::vector<const Appointment*> out;
    for (const Appointment& a : manager_.database().appointments())
        if (a.getPatientId() == patientId)
            out.push_back(&a);
    return out;
}

std::vector<const Appointment*> AppointmentController::appointmentsOn(
    const std::string& date) const
{
    std::vector<const Appointment*> out;
    for (const Appointment& a : manager_.database().appointments()) {
        if (a.getStatus() == AppointmentStatus::Scheduled && a.getDate() == date)
            out.push_back(&a);
    }
    return out;
}

} // namespace controller
} // namespace scapms
