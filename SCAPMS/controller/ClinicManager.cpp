#include "ClinicManager.h"

#include <algorithm>
#include <ctime>
#include <sstream>

#include "../model/GeneralPatient.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/IDGenerator.h"

namespace scapms {
namespace controller {

using namespace scapms::model;
using scapms::utils::IDGenerator;

// ---------- construction ----------

ClinicManager::ClinicManager()
{
    std::fill(std::begin(recentPatients_), std::end(recentPatients_), nullptr);
}

// ---------- validation helpers ----------

static void validateCommonFields(const std::string& name, int age,
                                 const std::string& contact)
{
    if (name.empty())
        throw utils::InvalidPatientData("Patient name must not be empty.");
    if (age < 0 || age > 120)
        throw utils::InvalidPatientData("Patient age must be between 0 and 120.");
    if (contact.empty())
        throw utils::InvalidPatientData("Contact information must not be empty.");
}

static int parseSeverity(const std::string& raw)
{
    int sev = 0;
    try {
        sev = std::stoi(raw);
    } catch (const std::exception&) {
        throw utils::InvalidPatientData(
            "Emergency severity must be a number between 1 and 5.");
    }
    if (sev < 1 || sev > 5)
        throw utils::InvalidPatientData(
            "Emergency severity must be between 1 and 5.");
    return sev;
}

static std::unique_ptr<PatientRecord> makePatient(const std::string& type,
                                                  const std::string& id,
                                                  const std::string& name,
                                                  int age,
                                                  const std::string& contact,
                                                  const std::string& extra1,
                                                  const std::string& extra2)
{
    if (type == "GENERAL")
        return std::make_unique<GeneralPatient>(id, name, age, contact, extra1);
    if (type == "EMERGENCY")
        return std::make_unique<EmergencyPatient>(id, name, age, contact,
                                                  parseSeverity(extra1), extra2);
    if (type == "FOLLOWUP")
        return std::make_unique<FollowUpPatient>(id, name, age, contact,
                                                 extra1, extra2);
    throw utils::InvalidPatientData("Unknown patient type: " + type);
}

// ---------- patient operations ----------

std::string ClinicManager::addPatient(const std::string& type,
                                      const std::string& name,
                                      int age,
                                      const std::string& contact,
                                      const std::string& extra1,
                                      const std::string& extra2)
{
    validateCommonFields(name, age, contact);

    const std::string id = IDGenerator::nextPatientId();
    if (db_.findPatientRaw(id))                    // paranoid duplicate check
        throw utils::DuplicateIdError(id);

    PatientPtr patient = makePatient(type, id, name, age, contact, extra1, extra2);
    PatientRecord* raw = patient.get();

    db_.addPatient(std::move(patient));
    touchCache(raw);
    markDataUnsorted();
    return id;
}

void ClinicManager::editPatient(const std::string& patientId,
                               const std::string& name,
                               int age,
                               const std::string& contact,
                               const std::string& extra1,
                               const std::string& extra2)
{
    PatientRecord* p = db_.findPatientRaw(patientId);
    if (!p)
        throw utils::ClinicError("Patient not found: " + patientId);

    validateCommonFields(name, age, contact);

    // --- register the undo snapshot BEFORE modifying the record ---
    UndoOperation op;
    op.type        = UndoType::ModifyPatient;
    op.patientId   = patientId;
    op.description = "Modify patient record " + patientId + " (" + p->getName() + ")";
    op.patientSnapshot = p->clone();
    undoStack_.push(std::move(op));

    p->setName(name);
    p->setAge(age);
    p->setContact(contact);

    // Type specific fields - dynamic_cast is safe here because each
    // concrete class knows how to store its own extras.
    if (auto* g = dynamic_cast<GeneralPatient*>(p)) {
        g->setReasonForVisit(extra1);
    } else if (auto* e = dynamic_cast<EmergencyPatient*>(p)) {
        e->setSeverityLevel(parseSeverity(extra1));
        e->setDescription(extra2);
    } else if (auto* f = dynamic_cast<FollowUpPatient*>(p)) {
        f->setLastVisitDate(extra1);
        f->setReviewNote(extra2);
    }

    touchCache(p);
    markDataUnsorted();
}

void ClinicManager::deletePatient(const std::string& patientId)
{
    PatientRecord* p = db_.findPatientRaw(patientId);
    if (!p)
        throw utils::ClinicError("Patient not found: " + patientId);

    // Keep the patient's appointments too so Undo can restore everything.
    std::vector<Appointment> linkedAppointments;
    for (const Appointment& a : db_.appointments())
        if (a.getPatientId() == patientId)
            linkedAppointments.push_back(a);

    // Snapshot patient name + clone BEFORE the record is removed from the DB
    // (p dangles after db_.removePatient()).
    std::string patientName = p->getName();
    PatientPtr patientSnapshot = p->clone();

    // Remove linked appointments (after the patient snapshot was taken).
    for (const Appointment& a : linkedAppointments)
        db_.removeAppointment(a.getId());

    invalidateCache(p);
    db_.removePatient(patientId);
    removeFromQueue(patientId);
    arrivalEpoch_.erase(patientId);
    markDataUnsorted();

    // Push CancelAppointment entries FIRST so they sit below DeletePatient
    // on the stack. Undo pops DeletePatient last (restoring the patient
    // first), then each appointment is re-linked to the now-restored patient.
    if (!linkedAppointments.empty()) {
        for (const Appointment& a : linkedAppointments) {
            UndoOperation apptOp;
            apptOp.type        = UndoType::CancelAppointment;
            apptOp.patientId   = patientId;
            apptOp.appointmentId = a.getId();
            apptOp.description = "(auto) remove appointment " + a.getId()
                                 + " linked to deleted patient " + patientId;
            apptOp.appointmentSnapshot = a;
            undoStack_.push(std::move(apptOp));
        }
    }

    UndoOperation op;
    op.type        = UndoType::DeletePatient;
    op.patientId   = patientId;
    op.description = "Delete patient " + patientId + " (" + patientName + ")";
    op.patientSnapshot = std::move(patientSnapshot);
    undoStack_.push(std::move(op));
}

PatientRecord* ClinicManager::getPatient(const std::string& patientId)
{
    if (patientId.empty())
        throw utils::InvalidSearchInput("Patient ID is empty.");
    PatientRecord* p = db_.findPatientRaw(patientId);
    if (p)
        touchCache(p); // frequently accessed records land in the cache
    return p;
}

// ---------- undo (std::stack) ----------

std::string ClinicManager::undoLast()
{
    if (undoStack_.empty())
        throw utils::ClinicError("There is no administrative operation to undo.");

    UndoOperation op = std::move(undoStack_.top());
    undoStack_.pop();

    switch (op.type) {
    case UndoType::DeletePatient:
        if (op.patientSnapshot &&
            !db_.findPatientRaw(op.patientSnapshot->getPatientId())) {
            PatientRecord* raw = op.patientSnapshot.get();
            db_.insertPatient(std::move(op.patientSnapshot));
            touchCache(raw);
        }
        break;

    case UndoType::ModifyPatient:
        if (op.patientSnapshot) {
            const std::string id = op.patientSnapshot->getPatientId();
            PatientRecord* current = db_.findPatientRaw(id);
            PatientRecord* raw = op.patientSnapshot.get();
            if (current)
                db_.removePatient(id);          // discards the edited version
            db_.insertPatient(std::move(op.patientSnapshot));
            touchCache(raw);
        }
        break;

    case UndoType::CancelAppointment:
        // Restore the appointment to the status it had before it was
        // cancelled/removed and re-link it with the patient record.
        if (Appointment* a = db_.findAppointment(op.appointmentId)) {
            a->setStatus(op.appointmentSnapshot.getStatus());
        } else {
            db_.addAppointment(op.appointmentSnapshot);
        }
        if (PatientRecord* p = db_.findPatientRaw(op.patientId))
            p->addAppointmentId(op.appointmentId);
        break;
    }

    markDataUnsorted();
    return op.description;
}

std::string ClinicManager::peekUndo() const
{
    if (undoStack_.empty())
        return "";
    return undoStack_.top().description;
}

// ---------- waiting queue (std::queue) ----------

void ClinicManager::enqueuePatient(const std::string& patientId)
{
    if (!db_.findPatientRaw(patientId))
        throw utils::ClinicError("Cannot queue unknown patient: " + patientId);

    if (isInQueue(patientId))
        throw utils::ClinicError("Patient " + patientId +
                                 " is already in the waiting queue.");

    arrivalEpoch_[patientId] = static_cast<long long>(std::time(nullptr));
    waitQueue_.push(patientId);
}

std::string ClinicManager::callNextPatient()
{
    if (waitQueue_.empty())
        throw utils::QueueEmptyError();

    const std::string id = waitQueue_.front();
    waitQueue_.pop();
    arrivalEpoch_.erase(id);
    return id;
}

std::vector<std::string> ClinicManager::queueSnapshot() const
{
    // std::queue has no iterators by design, so we copy it and drain the
    // copy - the original queue stays untouched.
    std::vector<std::string> out;
    std::queue<std::string> copy = waitQueue_;
    while (!copy.empty()) {
        out.push_back(copy.front());
        copy.pop();
    }
    return out;
}

bool ClinicManager::isInQueue(const std::string& patientId) const
{
    std::queue<std::string> copy = waitQueue_;
    while (!copy.empty()) {
        if (copy.front() == patientId)
            return true;
        copy.pop();
    }
    return false;
}

long long ClinicManager::frontWaitSeconds() const
{
    if (waitQueue_.empty())
        return 0;
    auto it = arrivalEpoch_.find(waitQueue_.front());
    if (it == arrivalEpoch_.end())
        return 0;
    return std::max<long long>(0,
        static_cast<long long>(std::time(nullptr)) - it->second);
}

long long ClinicManager::waitSecondsAt(std::size_t position) const
{
    std::vector<std::string> ids = queueSnapshot();
    if (position >= ids.size())
        return 0;
    auto it = arrivalEpoch_.find(ids[position]);
    if (it == arrivalEpoch_.end())
        return 0;
    return std::max<long long>(0,
        static_cast<long long>(std::time(nullptr)) - it->second);
}

void ClinicManager::removeFromQueue(const std::string& patientId)
{
    // std::queue does not support removal from the middle: rebuild it
    // without the removed patient.
    std::queue<std::string> rebuilt;
    while (!waitQueue_.empty()) {
        if (waitQueue_.front() != patientId)
            rebuilt.push(waitQueue_.front());
        waitQueue_.pop();
    }
    waitQueue_.swap(rebuilt);
}

// ---------- recent-records cache (fixed-size array, MRU order) ----------

void ClinicManager::touchCache(PatientRecord* record)
{
    if (!record)
        return;

    // Already cached? Move it to the front (most recently used).
    for (int i = 0; i < cacheCount_; ++i) {
        if (recentPatients_[i] == record) {
            for (int j = i; j > 0; --j)
                recentPatients_[j] = recentPatients_[j - 1];
            recentPatients_[0] = record;
            return;
        }
    }

    // New entry: shift everything one slot to the right and insert at 0.
    if (cacheCount_ < CacheCapacity)
        ++cacheCount_;
    for (int i = cacheCount_ - 1; i > 0; --i)
        recentPatients_[i] = recentPatients_[i - 1];
    recentPatients_[0] = record;
}

void ClinicManager::invalidateCache(const PatientRecord* record)
{
    for (int i = 0; i < cacheCount_; ++i) {
        if (recentPatients_[i] == record) {
            for (int j = i; j < cacheCount_ - 1; ++j)
                recentPatients_[j] = recentPatients_[j + 1];
            recentPatients_[cacheCount_ - 1] = nullptr;
            --cacheCount_;
            return;
        }
    }
}

std::vector<const PatientRecord*> ClinicManager::recentRecords() const
{
    std::vector<const PatientRecord*> out;
    for (int i = 0; i < cacheCount_; ++i)
        out.push_back(recentPatients_[i]);
    return out;
}

// ---------- polymorphic processing ----------

void ClinicManager::processAllPatients(std::ostream& os) const
{
    // Runtime polymorphism: the loop only knows the ABSTRACT base type,
    // but each record's own displayInfo() is executed.
    for (const PatientPtr& p : db_.patients()) {
        p->displayInfo(os); // virtual call through base-class pointer
        os << "\n";
    }
}

// ---------- sorting ----------

void ClinicManager::applySort(SortAlgorithm algo,
                              std::vector<PatientPtr>& data,
                              SearchKey key, const ClinicDatabase& db)
{
    switch (algo) {
    case SortAlgorithm::Selection: SearchEngine::selectionSort(data, key, db); break;
    case SortAlgorithm::Insertion: SearchEngine::insertionSort(data, key, db); break;
    case SortAlgorithm::Merge:     SearchEngine::mergeSort(data, key, db);     break;
    }
}

void ClinicManager::sortPatients(SortAlgorithm algo, SearchKey key)
{
    applySort(algo, db_.patients(), key, db_);
    sortedByKey_ = static_cast<int>(key);

    const char* algoNames[] = { "Selection Sort", "Insertion Sort", "Merge Sort" };
    std::ostringstream os;
    os << algoNames[static_cast<int>(algo)] << " by " << SearchEngine::keyName(key);
    lastSortDescription_ = os.str();
}

bool ClinicManager::isSortedBy(SearchKey key) const
{
    return sortedByKey_ == static_cast<int>(key);
}

void ClinicManager::markDataUnsorted()
{
    sortedByKey_ = -1;
}

// ---------- persistence ----------

void ClinicManager::saveFiles(const std::string& directory)
{
    std::vector<std::string> patientLines;
    patientLines.push_back("#SCAPMS-PATIENTS-V1");
    for (const PatientPtr& p : db_.patients())
        patientLines.push_back(p->serialize()); // virtual call per type
    utils::FileHandler::writeLines(directory + "/patients.csv", patientLines);

    std::vector<std::string> apptLines;
    apptLines.push_back("#SCAPMS-APPOINTMENTS-V1");
    for (const Appointment& a : db_.appointments())
        apptLines.push_back(a.serialize());
    utils::FileHandler::writeLines(directory + "/appointments.csv", apptLines);
}

ClinicManager::LoadResult ClinicManager::loadFiles(const std::string& directory)
{
    LoadResult result;
    clearAll();
    IDGenerator::reset();

    // ----- patients -----
    std::vector<std::string> patientLines =
        utils::FileHandler::readLines(directory + "/patients.csv");

    for (const std::string& line : patientLines) {
        try {
            std::vector<std::string> f = splitCsv(line);
            if (f.size() < 5)
                throw utils::ClinicError("too few fields");

            const std::string& type = f[0];
            std::string extra1, extra2;

            if (type == "GENERAL") {           // GENERAL,id,name,age,contact,reason
                extra1 = f.size() > 5 ? f[5] : "";
            } else if (type == "EMERGENCY") {  // EMERGENCY,id,name,age,contact,sev,desc
                extra1 = f.size() > 5 ? f[5] : "3";
                extra2 = f.size() > 6 ? f[6] : "";
            } else if (type == "FOLLOWUP") {   // FOLLOWUP,id,name,age,contact,last,note,count
                extra1 = f.size() > 5 ? f[5] : "";
                extra2 = f.size() > 6 ? f[6] : "";
            } else {
                throw utils::ClinicError("unknown patient type '" + type + "'");
            }

            PatientPtr patient = makePatient(type, f[1], f[2], std::stoi(f[3]),
                                             f[4], extra1, extra2);
            if (type == "FOLLOWUP" && f.size() > 7) {
                if (auto* fup = dynamic_cast<FollowUpPatient*>(patient.get())) {
                    try { fup->setFollowUpCount(static_cast<int>(std::stoul(f[7]))); }
                    catch (const std::exception&) {}
                }
            }
            IDGenerator::syncWithPatientId(f[1]);
            db_.addPatient(std::move(patient));
        } catch (const std::exception& e) {
            result.warnings.push_back("Skipped patient line (" +
                                      std::string(e.what()) + "): " + line);
        }
    }
    result.patientCount = static_cast<int>(db_.patientCount());

    // ----- appointments -----
    std::vector<std::string> apptLines;
    try {
        apptLines = utils::FileHandler::readLines(directory + "/appointments.csv");
    } catch (const utils::FileOperationError&) {
        apptLines.clear(); // appointments file is optional
    }

    for (const std::string& line : apptLines) {
        Appointment a;
        if (!Appointment::deserialize(line, a)) {
            result.warnings.push_back("Skipped appointment line: " + line);
            continue;
        }
        IDGenerator::syncWithAppointmentId(a.getId());
        // Only attach appointments whose patient actually exists.
        if (db_.findPatientRaw(a.getPatientId()))
            db_.addAppointment(a); // also re-establishes the association
    }
    result.appointmentCount = static_cast<int>(db_.appointments().size());

    markDataUnsorted();
    return result;
}

void ClinicManager::clearAll()
{
    db_.clear();

    std::queue<std::string> emptyQueue;
    waitQueue_.swap(emptyQueue);
    std::stack<UndoOperation> emptyStack;
    undoStack_.swap(emptyStack);

    arrivalEpoch_.clear();
    std::fill(std::begin(recentPatients_), std::end(recentPatients_), nullptr);
    cacheCount_ = 0;
    sortedByKey_ = -1;
    lastSortDescription_.clear();
    IDGenerator::reset();
}

// ---------- reports ----------

ClinicManager::Statistics ClinicManager::computeStatistics() const
{
    Statistics s;
    s.totalPatients = static_cast<int>(db_.patientCount());

    for (const PatientPtr& p : db_.patients()) {
        const std::string t = p->getTypeName(); // virtual dispatch
        if (t == "General")         ++s.generalPatients;
        else if (t == "Emergency")  ++s.emergencyPatients;
        else if (t == "Follow-Up")  ++s.followUpPatients;
    }

    s.totalAppointments = static_cast<int>(db_.appointments().size());
    for (const Appointment& a : db_.appointments()) {
        switch (a.getStatus()) {
        case AppointmentStatus::Scheduled: ++s.scheduledAppointments; break;
        case AppointmentStatus::Completed: ++s.completedAppointments; break;
        case AppointmentStatus::Cancelled: ++s.cancelledAppointments; break;
        }
    }

    s.waitingQueueSize = static_cast<int>(waitQueue_.size());
    return s;
}

void ClinicManager::pushCancelledAppointment(const std::string& appointmentId,
                                             const Appointment& snapshot,
                                             const std::string& patientId)
{
    UndoOperation op;
    op.type        = UndoType::CancelAppointment;
    op.appointmentId = appointmentId;
    op.patientId   = patientId;
    op.description = "Cancel appointment " + appointmentId;
    op.appointmentSnapshot = snapshot;
    undoStack_.push(std::move(op));
}

} // namespace controller
} // namespace scapms
