#ifndef SCAPMS_UTILS_EXCEPTIONS_H
#define SCAPMS_UTILS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace scapms {
namespace utils {

/**
 * Custom exception hierarchy used by the controller, model and GUI layers.
 * All exceptions derive from ClinicError so the GUI can catch the whole
 * family with a single catch block when it wants a generic handler.
 */
class ClinicError : public std::runtime_error {
public:
    explicit ClinicError(const std::string& message)
        : std::runtime_error(message) {}
};

// Thrown when patient data fails validation (empty name, bad age ...).
class InvalidPatientData : public ClinicError {
public:
    explicit InvalidPatientData(const std::string& message)
        : ClinicError("Invalid patient information: " + message) {}
};

// Thrown when an ID already exists in the database.
class DuplicateIdError : public ClinicError {
public:
    explicit DuplicateIdError(const std::string& id)
        : ClinicError("ID already exists: " + id) {}
};

// Thrown when appointment data fails validation.
class InvalidAppointmentData : public ClinicError {
public:
    explicit InvalidAppointmentData(const std::string& message)
        : ClinicError("Invalid appointment data: " + message) {}
};

// Thrown when a file cannot be opened, read or written.
class FileOperationError : public ClinicError {
public:
    explicit FileOperationError(const std::string& message)
        : ClinicError("File operation failed: " + message) {}
};

// Thrown when a search request itself is malformed (empty keyword ...).
class InvalidSearchInput : public ClinicError {
public:
    explicit InvalidSearchInput(const std::string& message)
        : ClinicError("Invalid search input: " + message) {}
};

// Thrown when a queue operation is attempted on an empty queue.
class QueueEmptyError : public ClinicError {
public:
    QueueEmptyError()
        : ClinicError("The waiting queue is empty.") {}
};

} // namespace utils
} // namespace scapms

#endif // SCAPMS_UTILS_EXCEPTIONS_H
