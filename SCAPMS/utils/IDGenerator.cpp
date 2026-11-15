#include "IDGenerator.h"

#include <cctype>
#include <cstdio>

namespace scapms {
namespace utils {

unsigned long IDGenerator::patientCounter_     = 0;
unsigned long IDGenerator::appointmentCounter_ = 0;

std::string IDGenerator::format(const char* prefix, unsigned long n)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%s%04lu", prefix, n);
    return std::string(buf);
}

std::string IDGenerator::nextPatientId()
{
    return format("P", ++patientCounter_);
}

std::string IDGenerator::nextAppointmentId()
{
    return format("A", ++appointmentCounter_);
}

unsigned long IDGenerator::parseNumber(const std::string& id)
{
    // Strip every non-digit character and convert the rest.
    std::string digits;
    for (char c : id)
        if (std::isdigit(static_cast<unsigned char>(c)))
            digits += c;
    if (digits.empty())
        return 0;
    unsigned long value = 0;
    for (char c : digits)
        value = value * 10 + static_cast<unsigned long>(c - '0');
    return value;
}

void IDGenerator::syncWithPatientId(const std::string& id)
{
    unsigned long n = parseNumber(id);
    if (n > patientCounter_)
        patientCounter_ = n;
}

void IDGenerator::syncWithAppointmentId(const std::string& id)
{
    unsigned long n = parseNumber(id);
    if (n > appointmentCounter_)
        appointmentCounter_ = n;
}

void IDGenerator::reset()
{
    patientCounter_ = 0;
    appointmentCounter_ = 0;
}

} // namespace utils
} // namespace scapms
