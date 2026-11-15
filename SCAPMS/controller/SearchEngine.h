#ifndef SCAPMS_CONTROLLER_SEARCHENGINE_H
#define SCAPMS_CONTROLLER_SEARCHENGINE_H

#include <string>
#include <vector>

#include "../model/PatientRecord.h"
#include "../model/ClinicDatabase.h"

namespace scapms {
namespace controller {

// Fields the records can be sorted/searched by.
enum class SortKey { PatientId, PatientName, Age, AppointmentDate };

// Alias kept for readability where the key is used as a search field.
using SearchKey = SortKey;

/**
 * SearchEngine - ALL searching and sorting algorithms are implemented
 * manually here (no std::find / std::sort / std::binary_search) because
 * the project requirement is to demonstrate the algorithms themselves.
 *
 * Sorting choice justification (assignment requirement):
 *  - Selection Sort : simple, predictable O(n^2); fine for the small
 *    record counts a university clinic handles and easy to understand.
 *  - Insertion Sort : O(n^2) worst case but nearly O(n) when the data is
 *    already almost sorted (typical after adding a few new records to an
 *    old sorted file) - stable, so equal keys keep their relative order.
 *  - Merge Sort     : guaranteed O(n log n); the right choice once the
 *    record collection grows beyond a few thousand entries.
 */
class SearchEngine {
public:
    // ---------- manual searching algorithms ----------

    // Linear search for a Patient ID. Works on sorted AND unsorted data.
    // Returns the vector index of the record or -1 when not found.
    static int linearSearchById(const std::vector<model::PatientPtr>& data,
                                const std::string& id);

    // Binary search - the data MUST already be sorted by the searched key,
    // otherwise the result is undefined (the caller checks the sorted flag).
    static int binarySearchByKey(const std::vector<model::PatientPtr>& data,
                                 const std::string& target,
                                 SortKey key,
                                 const model::ClinicDatabase& db);

    // Recursive search: collects every patient whose name contains the
    // keyword (case-insensitive). Implemented recursively on the index.
    static std::vector<int> recursiveNameSearch(
        const std::vector<model::PatientPtr>& data,
        const std::string& keyword);

    // ---------- manual sorting algorithms ----------
    static void selectionSort(std::vector<model::PatientPtr>& data,
                              SortKey key, const model::ClinicDatabase& db);
    static void insertionSort(std::vector<model::PatientPtr>& data,
                              SortKey key, const model::ClinicDatabase& db);
    static void mergeSort(std::vector<model::PatientPtr>& data,
                          SortKey key, const model::ClinicDatabase& db);

    // Extracts a comparable string key for a record (zero-padded numbers,
    // lower-cased names, earliest scheduled appointment date/time).
    static std::string keyOf(const model::PatientRecord& patient,
                             SortKey key, const model::ClinicDatabase& db);

    // Convenience helpers used by the GUI for labels.
    static std::string keyName(SortKey key);
};

} // namespace controller
} // namespace scapms

#endif // SCAPMS_CONTROLLER_SEARCHENGINE_H
