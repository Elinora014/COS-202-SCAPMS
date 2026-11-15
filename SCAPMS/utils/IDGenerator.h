#ifndef SCAPMS_UTILS_IDGENERATOR_H
#define SCAPMS_UTILS_IDGENERATOR_H

#include <string>

namespace scapms {
namespace utils {

/**
 * IDGenerator - produces unique sequential identifiers.
 *
 * Patients:  P0001, P0002, ...   (fixed width so lexicographic order of
 *            the IDs equals numeric order - required by binary search)
 * Appointments: A0001, A0002, ...
 *
 * After loading data from disk, syncWith() advances the counters past the
 * highest existing ID so new IDs can never collide with loaded ones.
 */
class IDGenerator {
public:
    static std::string nextPatientId();
    static std::string nextAppointmentId();

    // Adjusts the counters so that all future IDs are larger than any ID
    // already present in the loaded data.
    static void syncWithPatientId(const std::string& id);
    static void syncWithAppointmentId(const std::string& id);

    static void reset(); // start from 1 again (New / Load operations)

private:
    static unsigned long parseNumber(const std::string& id);
    static std::string format(const char* prefix, unsigned long n);

    static unsigned long patientCounter_;
    static unsigned long appointmentCounter_;
};

} // namespace utils
} // namespace scapms

#endif // SCAPMS_UTILS_IDGENERATOR_H
