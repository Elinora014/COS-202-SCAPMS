#include "SearchEngine.h"

#include <cctype>
#include <cstdio>
#include <algorithm>

namespace scapms {
namespace controller {

using namespace scapms::model;

// ---------- small internal helpers ----------

static std::string toLower(const std::string& s)
{
    std::string out = s;
    for (char& c : out)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

static std::string padAge(const std::string& rawAge)
{
    try {
        int age = std::stoi(rawAge);
        if (age < 0) age = 0;
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%04d", age);
        return buf;
    } catch (const std::exception&) {
        return rawAge;
    }
}

// Brings a user-typed search target into the same normalised form that
// keyOf() produces, so binary search comparisons are consistent.
static std::string normalizeTarget(const std::string& target, SortKey key)
{
    switch (key) {
    case SortKey::PatientName:  return toLower(target);
    case SortKey::Age:          return padAge(target);
    case SortKey::PatientId:    return target;
    case SortKey::AppointmentDate: return target;
    }
    return target;
}

// ---------- key extraction ----------

std::string SearchEngine::keyOf(const PatientRecord& patient, SortKey key,
                                const ClinicDatabase& db)
{
    switch (key) {
    case SortKey::PatientId:
        return patient.getPatientId();

    case SortKey::PatientName:
        return toLower(patient.getName());

    case SortKey::Age: {
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%04d", patient.getAge());
        return buf;
    }

    case SortKey::AppointmentDate: {
        // Earliest still-scheduled appointment of this patient; patients
        // without one are pushed to the end of the list.
        std::string best = "9999-12-31 23:59";
        for (const Appointment& a : db.appointments()) {
            if (a.getPatientId() != patient.getPatientId())
                continue;
            if (a.getStatus() != AppointmentStatus::Scheduled)
                continue;
            if (a.dateTime() < best)
                best = a.dateTime();
        }
        return best;
    }
    }
    return "";
}

std::string SearchEngine::keyName(SortKey key)
{
    switch (key) {
    case SortKey::PatientId:        return "Patient ID";
    case SortKey::PatientName:      return "Patient Name";
    case SortKey::Age:              return "Age";
    case SortKey::AppointmentDate:  return "Appointment Date";
    }
    return "";
}

// ---------- manual searching algorithms ----------

// LINEAR SEARCH: scans every element from left to right.
// Used because it works on unsorted data and on a singly comparable key.
int SearchEngine::linearSearchById(const std::vector<PatientPtr>& data,
                                   const std::string& id)
{
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (data[i]->getPatientId() == id)
            return static_cast<int>(i);
    }
    return -1;
}

// BINARY SEARCH: halves the search interval each step (O(log n)).
// Pre-condition: data is already sorted by the given key.
int SearchEngine::binarySearchByKey(const std::vector<PatientPtr>& data,
                                    const std::string& target,
                                    SortKey key,
                                    const ClinicDatabase& db)
{
    const std::string want = normalizeTarget(target, key);
    int low  = 0;
    int high = static_cast<int>(data.size()) - 1;

    while (low <= high) {
        const int mid = low + (high - low) / 2;   // avoids overflow
        const std::string midKey = keyOf(*data[mid], key, db);

        if (midKey == want) {
            // A sort key may not be unique (names, dates...). Walk left to
            // the first equal element so a deterministic index is returned.
            int first = mid;
            while (first > 0 && keyOf(*data[first - 1], key, db) == want)
                --first;
            return first;
        } else if (midKey < want) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

// RECURSIVE SEARCH helper: processes index i, then recurses to i + 1.
static void recursiveNameSearchHelper(const std::vector<PatientPtr>& data,
                                      const std::string& keywordLower,
                                      std::size_t index,
                                      std::vector<int>& hits)
{
    if (index >= data.size())
        return;

    if (toLower(data[index]->getName()).find(keywordLower) != std::string::npos)
        hits.push_back(static_cast<int>(index));

    recursiveNameSearchHelper(data, keywordLower, index + 1, hits);
}

std::vector<int> SearchEngine::recursiveNameSearch(
    const std::vector<PatientPtr>& data,
    const std::string& keyword)
{
    std::vector<int> hits;
    if (keyword.empty())
        return hits;
    recursiveNameSearchHelper(data, toLower(keyword), 0, hits);
    return hits;
}

// ---------- manual sorting algorithms ----------

// SELECTION SORT: repeatedly selects the smallest remaining key.
// O(n^2) but performs the fewest swaps - suitable when records are heavy
// (unique_ptr moves) and the data set is small, as in a clinic.
void SearchEngine::selectionSort(std::vector<PatientPtr>& data,
                                 SortKey key, const ClinicDatabase& db)
{
    const std::size_t n = data.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        std::size_t minPos = i;
        std::string minKey = keyOf(*data[i], key, db);
        for (std::size_t j = i + 1; j < n; ++j) {
            std::string k = keyOf(*data[j], key, db);
            if (k < minKey) {
                minKey = k;
                minPos = j;
            }
        }
        if (minPos != i)
            std::swap(data[i], data[minPos]);
    }
}

// INSERTION SORT: inserts each record into the already-sorted prefix.
// Almost O(n) on nearly-sorted data (the normal case after loading a
// sorted file and appending new patients) and stable.
void SearchEngine::insertionSort(std::vector<PatientPtr>& data,
                                 SortKey key, const ClinicDatabase& db)
{
    const std::size_t n = data.size();
    for (std::size_t i = 1; i < n; ++i) {
        PatientPtr current = std::move(data[i]);
        std::string currentKey = keyOf(*current, key, db);
        std::size_t j = i;
        while (j > 0 && keyOf(*data[j - 1], key, db) > currentKey) {
            data[j] = std::move(data[j - 1]);
            --j;
        }
        data[j] = std::move(current);
    }
}

// MERGE SORT internal: sorts [begin, end) recursively.
static void mergeRange(std::vector<PatientPtr>& data,
                       std::size_t begin, std::size_t mid, std::size_t end,
                       SortKey key, const ClinicDatabase& db)
{
    std::vector<PatientPtr> left;
    left.reserve(mid - begin);
    for (std::size_t i = begin; i < mid; ++i)
        left.push_back(std::move(data[i]));

    std::vector<PatientPtr> merged;
    merged.reserve(end - begin);

    std::size_t li = 0, ri = mid;
    while (li < left.size() && ri < end) {
        if (SearchEngine::keyOf(*left[li], key, db)
            <= SearchEngine::keyOf(*data[ri], key, db))
            merged.push_back(std::move(left[li++]));
        else
            merged.push_back(std::move(data[ri++]));
    }
    while (li < left.size())
        merged.push_back(std::move(left[li++]));
    while (ri < end)
        merged.push_back(std::move(data[ri++]));

    for (std::size_t i = 0; i < merged.size(); ++i)
        data[begin + i] = std::move(merged[i]);
}

static void mergeSortRange(std::vector<PatientPtr>& data,
                           std::size_t begin, std::size_t end,
                           SortKey key, const ClinicDatabase& db)
{
    if (end - begin < 2)
        return;
    const std::size_t mid = begin + (end - begin) / 2;
    mergeSortRange(data, begin, mid, key, db);
    mergeSortRange(data, mid, end, key, db);
    mergeRange(data, begin, mid, end, key, db);
}

// MERGE SORT: divide & conquer, guaranteed O(n log n) - chosen for large
// data sets where the quadratic algorithms would become too slow.
void SearchEngine::mergeSort(std::vector<PatientPtr>& data,
                             SortKey key, const ClinicDatabase& db)
{
    mergeSortRange(data, 0, data.size(), key, db);
}

} // namespace controller
} // namespace scapms
