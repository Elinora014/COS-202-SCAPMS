#ifndef SCAPMS_CONTROLLER_CLINICMANAGER_H
#define SCAPMS_CONTROLLER_CLINICMANAGER_H

#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <memory>
#include <ostream>

#include "../model/ClinicDatabase.h"
#include "../model/PatientRecord.h"
#include "../model/Appointment.h"
#include "SearchEngine.h"

namespace scapms {
namespace controller {

// Algorithms the GUI dropdown can offer (each one is implemented manually
// in SearchEngine).
enum class SortAlgorithm { Selection, Insertion, Merge };

/**
 * ClinicManager - the central controller ("business logic") of the system.
 *
 * Required data structures used here:
 *  - std::vector  : the patient collection (inside ClinicDatabase).
 *  - std::queue   : the FIFO waiting queue of patient IDs.
 *  - std::stack   : Undo Last Administrative Operation.
 *  - fixed array  : LRU-style cache of the 10 most recently accessed
 *                   patient records (PatientRecord* recentPatients[10]).
 */
class ClinicManager {
public:
    static constexpr int CacheCapacity = 10;

    struct LoadResult {
        int patientCount   = 0;
        int appointmentCount = 0;
        std::vector<std::string> warnings; // malformed lines that were skipped
    };

    struct Statistics {
        int totalPatients   = 0;
        int generalPatients = 0;
        int emergencyPatients = 0;
        int followUpPatients = 0;
        int totalAppointments = 0;
        int scheduledAppointments = 0;
        int completedAppointments = 0;
        int cancelledAppointments = 0;
        int waitingQueueSize = 0;
    };

    // Administrative operations that can be reversed with Undo.
    enum class UndoType { DeletePatient, ModifyPatient, CancelAppointment };

    ClinicManager();

    // ---------- patient operations ----------
    // extra1/extra2 carry type specific data:
    //   GENERAL    : extra1 = reason for visit
    //   EMERGENCY  : extra1 = severity (1-5), extra2 = case description
    //   FOLLOWUP   : extra1 = last visit date, extra2 = review note
    // Throws utils::InvalidPatientData / utils::DuplicateIdError.
    // Returns the newly assigned patient ID.
    std::string addPatient(const std::string& type,
                           const std::string& name,
                           int age,
                           const std::string& contact,
                           const std::string& extra1 = "",
                           const std::string& extra2 = "");

    // Editing an existing patient (registers an undo snapshot first).
    void editPatient(const std::string& patientId,
                     const std::string& name,
                     int age,
                     const std::string& contact,
                     const std::string& extra1 = "",
                     const std::string& extra2 = "");

    // Deleting a patient (snapshot kept for undo; queue/cache cleaned).
    void deletePatient(const std::string& patientId);

    // Non-owning observation pointer; also refreshes the recent-records
    // cache and throws utils::InvalidSearchInput when the ID is blank or
    // utils::ClinicError("not found") style message via nullptr handling
    // is left to the caller (the GUI checks for null).
    model::PatientRecord* getPatient(const std::string& patientId);

    // ---------- undo (std::stack) ----------
    bool canUndo() const { return !undoStack_.empty(); }
    std::string undoLast();            // returns description of undone op
    std::string peekUndo() const;      // description of top entry

    // ---------- waiting queue (std::queue) ----------
    void enqueuePatient(const std::string& patientId);
    std::string callNextPatient();     // throws utils::QueueEmptyError
    std::vector<std::string> queueSnapshot() const;
    bool isInQueue(const std::string& patientId) const;
    // Seconds the front patient has been waiting (0 when queue empty).
    long long frontWaitSeconds() const;
    // Waiting time of the patient at the given queue position (0-indexed).
    long long waitSecondsAt(std::size_t position) const;

    // ---------- recent-records cache (fixed array) ----------
    std::vector<const model::PatientRecord*> recentRecords() const;

    // ---------- polymorphic processing ----------
    // Iterates the whole collection through the BASE CLASS pointer and
    // lets every record print itself - demonstrates runtime polymorphism.
    void processAllPatients(std::ostream& os) const;

    // ---------- sorting / search-state helpers ----------
    void sortPatients(SortAlgorithm algo, SearchKey key);
    // True when the vector is currently sorted by `key` (binary search
    // may only be used then); any data mutation resets this state.
    bool isSortedBy(SearchKey key) const;
    // Applies the selected algorithm to the vector.
    static void applySort(SortAlgorithm algo,
                          std::vector<model::PatientPtr>& data,
                          SearchKey key, const model::ClinicDatabase& db);
    std::string lastSortDescription() const { return lastSortDescription_; }

    // ---------- persistence ----------
    void saveFiles(const std::string& directory);   // throws FileOperationError
    LoadResult loadFiles(const std::string& directory);
    void clearAll(); // in-memory reset (File > New)

    // ---------- reports ----------
    Statistics computeStatistics() const;

    // direct (read-mostly) access for the other controllers / GUI
    model::ClinicDatabase& database() { return db_; }
    const model::ClinicDatabase& database() const { return db_; }

    // Used by AppointmentController to register undoable operations.
    void pushCancelledAppointment(const std::string& appointmentId,
                                  const model::Appointment& snapshot,
                                  const std::string& patientId);

private:
    struct UndoOperation {
        UndoType type;
        std::string description;
        std::string patientId;
        std::string appointmentId;
        model::PatientPtr patientSnapshot;   // for Delete/Modify patient
        model::Appointment appointmentSnapshot; // for CancelAppointment
    };

    void touchCache(model::PatientRecord* record);
    void invalidateCache(const model::PatientRecord* record);
    void removeFromQueue(const std::string& patientId);
    void markDataUnsorted();

    model::ClinicDatabase db_;

    std::queue<std::string> waitQueue_;                    // QUEUE
    std::map<std::string, long long> arrivalEpoch_;        // enqueue time
    std::stack<UndoOperation> undoStack_;                  // STACK
    model::PatientRecord* recentPatients_[CacheCapacity];  // ARRAY cache
    int cacheCount_ = 0;

    // Which key the patient vector is currently sorted by (-1 = unsorted).
    int sortedByKey_ = -1;
    std::string lastSortDescription_;
};

} // namespace controller
} // namespace scapms

#endif // SCAPMS_CONTROLLER_CLINICMANAGER_H
